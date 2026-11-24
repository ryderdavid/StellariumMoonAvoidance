import concurrent.futures,contextlib,copy,datetime,hashlib,io,json,os,pathlib,shlex,shutil,subprocess,sys,tempfile,unittest
from unittest.mock import patch
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]))
import journal
import lifecycle as gate

def bash():
    if os.name=='nt':
        candidate=pathlib.Path(shutil.which('git')).resolve().parents[1]/'bin/bash.exe'
        if candidate.exists():return str(candidate)
    return shutil.which('bash') or 'bash'

class JournalTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup)
        self.out=pathlib.Path(self.tmp.name)/'outbox'
        self.p={'repo':'org/repo','stage':'stage-one','kind':'start','sequence':1,'payload_digest':'a'*64,'timestamp':'2026-11-24T00:00:00Z'}
        self.adapter=pathlib.Path(self.tmp.name)/'adapter.py'
        self.adapter.write_text('import sys,json\ne=json.load(sys.stdin)\nprint(json.dumps({"event_id":e["event_id"],"digest":e["digest"],"readback":True,"indexed":True}))\n')
        self.command=[sys.executable,str(self.adapter)]
    def test_deterministic_id_and_idempotent_retry(self):
        a=journal.deliver(self.p,self.out,self.command);b=journal.deliver(self.p,self.out,self.command)
        self.assertEqual(a,b);self.assertTrue(journal.verified(a,self.out))
        self.assertEqual(len(list(self.out.glob('*.json'))),2)
    def test_offline_durable_failure_and_retry(self):
        with self.assertRaisesRegex(ValueError,'pending'):journal.deliver(self.p,self.out,[])
        self.assertEqual(len(list(self.out.glob('*.json'))),1)
        r=journal.deliver(self.p,self.out,self.command);self.assertTrue(journal.verified(r,self.out))
        if os.name!='nt':  # Windows uses inherited user-profile ACLs, not POSIX modes.
            self.assertEqual(self.out.stat().st_mode & 0o777,0o700)
            self.assertEqual((self.out/(r['event_id']+'.json')).stat().st_mode & 0o777,0o600)
    def test_failed_adapter_does_not_acknowledge(self):
        with self.assertRaisesRegex(ValueError,'pending'):journal.deliver(self.p,self.out,[sys.executable,'-c','raise SystemExit(1)'])
        self.assertFalse(list(self.out.glob('*.ack.json')))
    def test_same_identity_conflict(self):
        journal.deliver(self.p,self.out,self.command)
        changed=dict(self.p,timestamp='2026-11-25T00:00:00Z')
        with self.assertRaisesRegex(ValueError,'identity'):journal.deliver(changed,self.out,self.command)
    def test_parallel_stage_events(self):
        payloads=[dict(self.p,stage='stage-'+str(i)) for i in range(8)]
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            receipts=list(pool.map(lambda p:journal.deliver(p,self.out,self.command),payloads))
        self.assertEqual(len({r['event_id'] for r in receipts}),8)
        self.assertTrue(all(journal.verified(r,self.out) for r in receipts))
    def test_parallel_retry_of_same_event(self):
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            receipts=list(pool.map(lambda _:journal.deliver(self.p,self.out,self.command),range(8)))
        self.assertTrue(all(r==receipts[0] for r in receipts))
        self.assertEqual(len(list(self.out.glob('*.json'))),2)
    def test_bad_readback_or_index_rejected(self):
        for bad in ['{}','{"readback":true,"indexed":false}']:
            with self.subTest(bad=bad),self.assertRaises(ValueError):
                journal.deliver(self.p,self.out,[sys.executable,'-c','print('+repr(bad)+')'])
    def test_payload_tampering_invalidates_private_ack(self):
        r=journal.deliver(self.p,self.out,self.command)
        f=self.out/(r['event_id']+'.json');e=json.loads(f.read_text());e['payload']['repo']='other';f.write_text(json.dumps(e))
        self.assertFalse(journal.verified(r,self.out))

class GateTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup)
        self.root=pathlib.Path(self.tmp.name);self.original=gate.ROOT;gate.ROOT=self.root
        self.addCleanup(setattr,gate,'ROOT',self.original)
        self.git('init','-q');self.git('config','user.email','test@example.invalid');self.git('config','user.name','Test')
        self.put('.engineering/policy.json',json.dumps({'repo':'org/repo','bead_prefix':'unit','openspec':'1.15.0','beads':'1.2.2'}))
        self.put('openspec/changes/bootstrap-governance/proposal.md','baseline proposal\n')
        self.put('AGENTS.md','contract\n');self.commit()
        self.base=self.git('rev-parse','HEAD')
        self.w={'schema':1,'stage':'stage-one','change':'bootstrap-governance','bead':'unit-abc.1','scope':'governance','base':self.base}
    def put(self,p,s):
        f=self.root/p;f.parent.mkdir(parents=True,exist_ok=True);f.write_text(s)
    def git(self,*a):return subprocess.check_output(['git',*a],cwd=self.root,text=True,stderr=subprocess.DEVNULL).strip()
    def commit(self):self.git('add','.');self.git('-c','core.hooksPath=/dev/null','commit','-qm','fixture')
    def test_missing_and_invalid_spec_or_bead(self):
        for field,val in [('bead',''),('bead','other-abc'),('change','missing'),('change','../bad')]:
            with self.subTest(field=field,val=val),self.assertRaises(AssertionError):gate.validate_record(dict(self.w,**{field:val}))
    def test_staged_record_not_unstaged_record(self):
        self.put('.engineering/work.json',json.dumps(dict(self.w,bead='bad')));self.git('add','.')
        self.put('.engineering/work.json',json.dumps(self.w))
        with self.assertRaises(AssertionError),patch.object(gate,'specs'):gate.precommit()
    def test_incremental_commit_does_not_need_final_receipt(self):
        self.put('.engineering/work.json',json.dumps(self.w));self.git('add','.')
        with patch.object(gate,'specs'):gate.precommit()
    def test_hygiene_private_or_database_rejected(self):
        for p,data in [('.beads/embeddeddolt/data',b'data'),('AGENTS.md',('/'+'Users/'+'person/'+'secret').encode()),('private/file',b'data')]:
            with self.subTest(p=p),self.assertRaises(AssertionError):gate.hygiene([p],lambda _:data)
    def test_product_scope_cannot_hide_product_diff(self):
        self.put('product.cpp','changed');self.commit()
        with self.assertRaisesRegex(AssertionError,'product diff'):gate.validate_record(self.w)
    def test_snapshot_matches_git_symlink_content(self):
        blob=subprocess.check_output(['git','hash-object','-w','--stdin'],cwd=self.root,input=b'AGENTS.md').decode().strip()
        self.git('update-index','--add','--cacheinfo','120000,'+blob+',link')
        # Git's stored object is a target string even on a Windows checkout.
        self.assertEqual(subprocess.check_output(['git','show',':link'],cwd=self.root),b'AGENTS.md')
        if os.name!='nt':
            (self.root/'link').symlink_to('AGENTS.md');self.commit()
            self.assertEqual(gate.snapshot(),gate.snapshot('HEAD'))
    def test_stale_evidence_rejected(self):
        w=dict(self.w,evidence={'payload_digest':'f'*64,'commands':[{'command':'test','result':'passed'}]})
        with self.assertRaisesRegex(AssertionError,'stale'):gate.evidence(w)
    def test_receipt_covers_payload_and_requires_start(self):
        r={'schema':1,'event_id':'event-'+'a'*32,'digest':'a'*64,'state':'verified','kind':'complete','payload_digest':gate.snapshot('HEAD')}
        with self.assertRaisesRegex(AssertionError,'start'):gate.receipt_check(dict(self.w,journal=[r]),False)
        w=dict(self.w,journal=[dict(r,kind='start'),r]);gate.receipt_check(w,False)
        w['journal'][-1]['payload_digest']='b'*64
        with self.assertRaisesRegex(AssertionError,'cover revision'):gate.receipt_check(w,False)
    def test_superseded_completion_is_preserved_as_history(self):
        r={'schema':1,'event_id':'event-'+'a'*32,'digest':'a'*64,'state':'verified','kind':'complete','payload_digest':gate.snapshot('HEAD')}
        old=dict(r,event_id='event-'+'b'*32,payload_digest='c'*64)
        gate.receipt_check(dict(self.w,journal=[dict(r,kind='start'),old,r]),False)
    def test_public_claim_insufficient_for_trusted_completion(self):
        r={'schema':1,'event_id':'event-'+'a'*32,'digest':'a'*64,'state':'verified','kind':'complete','payload_digest':gate.snapshot('HEAD')}
        with patch.dict(os.environ,{'ENGINEERING_OUTBOX':str(self.root/'not-present')}),self.assertRaisesRegex(AssertionError,'acknowledgement'):
            gate.receipt_check(dict(self.w,journal=[dict(r,kind='start'),r]),True)
    def test_tracker_rejects_unclaimed_or_wrong_spec(self):
        for record in [{'status':'open','description':'bootstrap-governance'}, {'status':'in_progress','description':'different-change'}]:
            def result(*args,**kwargs):
                return 'bd version 1.2.2 (Homebrew)' if args[1]=='--version' else json.dumps([record])
            with self.subTest(record=record),patch.object(gate,'run',side_effect=result),self.assertRaises(AssertionError):gate.tracker(self.w)
    def test_tracker_sync_failure_remains_incomplete(self):
        def result(*args,**kwargs):
            if args[1]=='--version':return 'bd version 1.2.2 (Homebrew)'
            if args[1]=='show':return json.dumps([{'status':'in_progress','description':'bootstrap-governance'}])
            if args[2]=='push':raise subprocess.CalledProcessError(1,args)
            return ''
        with patch.object(gate,'run',side_effect=result),self.assertRaises(subprocess.CalledProcessError):gate.tracker(self.w)
    def test_no_verify_commit_is_caught_by_public_ci(self):
        self.put('.engineering/work.json',json.dumps(dict(self.w,change='absent')));self.commit()
        with self.assertRaisesRegex(AssertionError,'proposal'),patch.object(gate,'specs'):gate.check()
    def test_external_pr_can_be_pending_without_private_credentials(self):
        w=dict(self.w,evidence={'payload_digest':gate.snapshot('HEAD'),'commands':[{'command':'python3 -m unittest discover -s tools/engineering/tests -v','result':'passed'}]})
        self.put('.engineering/work.json',json.dumps(w));self.commit()
        with patch.object(gate,'specs'):gate.check(external=True)
        with self.assertRaisesRegex(AssertionError,'start'),patch.object(gate,'specs'):gate.check(external=False)
    def test_hooks_preserve_existing_hooks_and_push_stdin(self):
        prior=self.root/'old-hooks';prior.mkdir()
        for name in ['pre-commit','pre-push','post-checkout']:
            f=prior/name;f.write_text('#!/bin/sh\ncat > '+shlex.quote((self.root/(name+'.input')).as_posix())+'\necho '+name+' >> '+shlex.quote((self.root/'calls').as_posix())+'\n');f.chmod(0o755)
        self.git('config','core.hooksPath',str(prior))
        gate.bootstrap_hooks();gate.bootstrap_hooks()
        script='import sys,pathlib\np=pathlib.Path("hook-input")\np.write_text(sys.stdin.read())\n'
        self.put('tools/engineering/lifecycle.py',script)
        hp=pathlib.Path(self.git('config','--get','core.hooksPath'))
        subprocess.run([bash(),(hp/'pre-push').as_posix(),'origin','url'],cwd=self.root,input='ref sha remote sha\n',text=True,check=True)
        self.assertEqual((self.root/'pre-push.input').read_text(),(self.root/'hook-input').read_text())
        subprocess.run([bash(),(hp/'post-checkout').as_posix()],cwd=self.root,check=True)
        self.assertIn('post-checkout',(self.root/'calls').read_text())
    def test_existing_hook_failure_stops_lifecycle(self):
        hooks=self.root/'old-hooks';hooks.mkdir();f=hooks/'pre-commit';f.write_text('#!/bin/sh\nexit 7\n');f.chmod(0o755)
        self.git('config','core.hooksPath',str(hooks));gate.bootstrap_hooks()
        p=subprocess.run([bash(),(pathlib.Path(self.git('config','--get','core.hooksPath'))/'pre-commit').as_posix()],cwd=self.root)
        self.assertEqual(p.returncode,7)

if __name__=='__main__':unittest.main()
