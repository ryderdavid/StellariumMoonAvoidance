#!/usr/bin/env python3
"""Versioned, portable lifecycle gates (Python 3.11+, Git, OpenSpec, Beads)."""
import argparse,datetime,hashlib,json,os,pathlib,re,shlex,shutil,subprocess,sys,tempfile
from journal import canonical,deliver,verified
ROOT=pathlib.Path(__file__).resolve().parents[2]
HEX=re.compile(r'^[0-9a-f]{64}$')
SLUG=re.compile(r'^[a-z0-9]+(?:-[a-z0-9]+)*$')

def run(*args,cwd=None):
    command=[shutil.which(args[0]) or args[0],*args[1:]]
    return subprocess.check_output(command,cwd=cwd or ROOT,text=True,stderr=subprocess.PIPE).strip()

def git(*args):return run('git',*args)
def load(path):return json.loads((ROOT/path).read_text())
def write(path,data):
    path=ROOT/path;path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(json.dumps(data,indent=2)+'\n')
def policy():return load('.engineering/policy.json')
def excluded(p):return p=='.engineering/work.json' or p.startswith('.engineering/receipts/')

def snapshot(revision=None):
    if revision:
        names=git('ls-tree','-r','--name-only',revision).splitlines()
        read=lambda p:subprocess.check_output(['git','show',revision+':'+p],cwd=ROOT)
    else:
        names=git('ls-files','--cached','--others','--exclude-standard').splitlines()
        read=lambda p:os.readlink(ROOT/p).encode() if (ROOT/p).is_symlink() else (ROOT/p).read_bytes()
    h=hashlib.sha256()
    for p in sorted(set(names)):
        if excluded(p):continue
        if not revision and not (ROOT/p).is_file():continue
        h.update(p.encode()+b'\0'+read(p)+b'\0')
    return h.hexdigest()

def validate_record(w):
    p=policy()
    assert w['schema']==1 and SLUG.fullmatch(w['stage']),'invalid stage'
    assert re.fullmatch(re.escape(p['bead_prefix'])+r'-[a-z0-9]+(?:\.[0-9]+)*',w['bead']),'invalid bead'
    assert SLUG.fullmatch(w['change']),'invalid OpenSpec change'
    assert (ROOT/'openspec/changes'/w['change']/'proposal.md').is_file(),'missing OpenSpec proposal'
    assert w['scope'] in ('governance','product'),'invalid scope'
    if w['scope']=='governance':
        changed=git('diff','--name-only',w['base'],'HEAD').splitlines()
        assert all(governance_path(x) for x in changed),'product diff requires product evidence'
    return w

def governance_path(p):
    return p in ('AGENTS.md','.gitignore') or p.startswith(('openspec/','.engineering/','tools/engineering/','.beads/','.github/workflows/engineering.yml'))

def hygiene(paths,reader):
    for p in paths:
        assert not p.startswith('.beads/') or p in ('.beads/.gitignore','.beads/config.yaml','.beads/README.md'),'database/runtime file tracked'
        assert not any(x in pathlib.PurePosixPath(p).parts for x in ('node_modules','__pycache__','.private','private')),'generated/private file tracked'
        assert not p.endswith(('.pyc','.pem','.key','.p12','.db','.sqlite3')),'generated/secret file tracked'
        try:s=reader(p).decode('utf-8')
        except UnicodeDecodeError:continue
        forbidden=['/' + r'Users/[^/\s]+/',r'Obsidian' + 'Vault',r'(?i)Resources/' + 'Wiki/',r'(?i)BEGIN (?:RSA |OPENSSH )?PRIVATE KEY',r'gh[pousr]_[A-Za-z0-9]{30,}']
        # Do not self-match detector source; assembled patterns avoid literal paths.
        for pattern in forbidden:
            assert not re.search(pattern,s),'private content in '+p

def specs(staged=False):
    assert run('openspec','--version')==policy()['openspec'],'OpenSpec version mismatch'
    if staged:
        with tempfile.TemporaryDirectory() as tmp:
            for p in git('ls-files','--cached','openspec').splitlines():
                f=pathlib.Path(tmp)/p;f.parent.mkdir(parents=True,exist_ok=True)
                f.write_bytes(subprocess.check_output(['git','show',':'+p],cwd=ROOT))
            run('openspec','validate','--all','--strict',cwd=tmp)
    else:run('openspec','validate','--all','--strict')

def precommit():
    paths=git('diff','--cached','--name-only','--diff-filter=ACMR').splitlines()
    if not paths:return
    run('git','diff','--cached','--check')
    read=lambda p:subprocess.check_output(['git','show',':'+p],cwd=ROOT)
    hygiene(paths,read)
    w=json.loads(read('.engineering/work.json'));validate_record(w)
    if w['scope']=='governance':assert all(governance_path(x) for x in git('diff','--cached','--name-only').splitlines()),'product staged diff requires product scope'
    # Linkage comes from the index, so unstaged metadata cannot authorize a commit.
    read('openspec/changes/'+w['change']+'/proposal.md')
    specs(staged=True)
    print('pre-commit: staged hygiene and OpenSpec linkage passed')

def evidence(w):
    assert w['evidence']['payload_digest']==snapshot('HEAD'),'stale test evidence'
    commands=w['evidence']['commands']
    assert commands and all(x['result']=='passed' for x in commands),'missing/failed test evidence'
    assert any(x['command']=='python3 -m unittest discover -s tools/engineering/tests -v' for x in commands),'governance tests required'
    if w['scope']=='product':assert w['evidence'].get('product_tests'),'applicable product test evidence required'

def receipt_check(w,trusted):
    receipts=w.get('journal',[])
    assert any(r['kind']=='start' for r in receipts),'missing stage start'
    completed=[r for r in receipts if r['kind']=='complete']
    assert completed,'missing completion receipt'
    for r in receipts:
        assert set(r)=={'schema','event_id','digest','state','kind','payload_digest'},'unsanitized receipt'
        assert r['schema']==1 and re.fullmatch('event-[0-9a-f]{32}',r['event_id']),'invalid event identity'
        assert r['kind'] in ('start','decision','failure','blocked','complete'),'invalid receipt kind'
        assert HEX.fullmatch(r['digest']) and HEX.fullmatch(r['payload_digest']),'invalid digest'
        assert r['state']=='verified','journal incomplete'
        if r['kind']=='complete':assert r['payload_digest']==snapshot('HEAD'),'receipt does not cover revision'
        if trusted:
            assert verified(r,os.environ['ENGINEERING_OUTBOX']),'private readback acknowledgement missing'

def tracker(w):
    assert run('bd','--version').split()[2]==policy()['beads'],'Beads version mismatch'
    record=json.loads(run('bd','show',w['bead'],'--json'))
    if isinstance(record,list):record=record[0]
    assert record['status'] in ('in_progress','closed'),'bead not claimed/complete'
    assert w['change'] in record['description'],'bead missing spec linkage'
    # Version-correct Dolt operations, never bd sync or force.
    run('bd','dolt','commit','-m','Lifecycle checkpoint')
    run('bd','dolt','pull')
    run('bd','dolt','push')

def check(trusted=False,base=None,external=False):
    w=validate_record(load('.engineering/work.json'));specs()
    paths=git('diff','--name-only','--diff-filter=ACMR',base or w['base'],'HEAD').splitlines()
    if w['scope']=='governance':
        assert all(governance_path(x) for x in git('diff','--name-only',base or w['base'],'HEAD').splitlines()),'product PR diff requires product scope'
    hygiene(paths,lambda p:subprocess.check_output(['git','show','HEAD:'+p],cwd=ROOT))
    run('git','diff','--check',base or w['base'],'HEAD')
    evidence(w)
    if external and not w.get('journal'):
        print('external PR: journal pending; trusted maintainer completion required before acceptance')
    else:receipt_check(w,trusted)
    if trusted:
        assert not git('status','--porcelain','--untracked-files=no'),'tracked work must be committed'
        tracker(w)
    print('trusted checkpoint passed' if trusted else 'public checkpoint passed (structure and revision binding only)')

def record_event(args):
    w=validate_record(load('.engineering/work.json'))
    payload=json.loads(pathlib.Path(args.details).read_text())
    required={'timestamp','repo','stage','kind','sequence','commit','pr','bead','spec','tests','product_changes','lessons','decisions','next_step','payload_digest'}
    assert required<=payload.keys(),'missing journal detail fields'
    assert payload['repo']==policy()['repo'] and payload['stage']==w['stage'],'event context mismatch'
    assert payload['kind'] in ('start','decision','failure','blocked','complete'),'invalid event kind'
    assert payload['bead']==w['bead'] and payload['spec']==w['change'],'event linkage mismatch'
    stamp=datetime.datetime.fromisoformat(payload['timestamp'].replace('Z','+00:00'))
    assert stamp.tzinfo is not None,'timestamp must include timezone'
    assert isinstance(payload['sequence'],int) and payload['sequence']>0,'invalid sequence'
    assert HEX.fullmatch(payload['payload_digest']),'invalid event payload digest'
    assert re.fullmatch('[0-9a-f]{40}',payload['commit']),'invalid event commit'
    git('cat-file','-e',payload['commit']+'^{commit}')
    outbox=pathlib.Path(os.environ['ENGINEERING_OUTBOX']).resolve()
    assert not outbox.is_relative_to(ROOT),'private outbox must be outside repository'
    receipt=deliver(payload,outbox,shlex.split(os.environ.get('ENGINEERING_JOURNAL_ADAPTER','')))
    prior=[r for r in w.get('journal',[]) if r['event_id']==receipt['event_id']]
    assert not prior or prior[0]==receipt,'receipt conflict'
    if not prior:w.setdefault('journal',[]).append(receipt)
    write('.engineering/work.json',w)
    print(receipt['event_id']+' verified')

def test_evidence():
    # Use actual process result and revision digest; do not accept hand-entered pass claims.
    run(sys.executable,'-m','unittest','discover','-s','tools/engineering/tests','-v')
    specs()
    w=load('.engineering/work.json')
    w['evidence']={'payload_digest':snapshot(),'commands':[
        {'command':'python3 -m unittest discover -s tools/engineering/tests -v','result':'passed'},
        {'command':'openspec validate --all --strict','result':'passed'}]}
    write('.engineering/work.json',w)
    print('test evidence recorded for '+w['evidence']['payload_digest'])

def bootstrap_hooks():
    # Keep original hooks/config intact. Delegating wrappers live in common Git dir.
    common=pathlib.Path(git('rev-parse','--path-format=absolute','--git-common-dir'))
    target=common/'engineering-hooks';target.mkdir(exist_ok=True)
    current=git('config','--get','core.hooksPath') if subprocess.run(['git','config','--get','core.hooksPath'],cwd=ROOT,stdout=subprocess.DEVNULL).returncode==0 else str(common/'hooks')
    saved=target/'previous.json'
    if saved.exists():current=json.loads(saved.read_text())['path']
    else:saved.write_text(json.dumps({'path':str((ROOT/pathlib.Path(current)).resolve())}))
    current=str((ROOT/pathlib.Path(current)).resolve())
    for name,cmd in [('pre-commit','pre-commit'),('pre-push','pre-push')]:
        prior=pathlib.Path(current)/name
        # pre-push stdin is replayed for both hooks, preserving existing Beads hooks.
        content='#!/bin/sh\nset -eu\n'
        if name=='pre-push':
            content+='input=$(mktemp)\ntrap \'rm -f "$input"\' EXIT HUP INT TERM\ncat > "$input"\n'
        if prior.is_file():
            content+='if [ -x '+shlex.quote(prior.as_posix())+' ]; then\n  '+shlex.quote(prior.as_posix())+' "$@"'+(' < "$input"' if name=='pre-push' else '')+'\nfi\n'
        content+='root=$(git rev-parse --show-toplevel)\nif command -v python3 >/dev/null 2>&1; then py=python3; else py=python; fi\n"$py" "$root/tools/engineering/lifecycle.py" '+cmd+(' < "$input"' if name=='pre-push' else '')+'\n'
        f=target/name;f.write_text(content);f.chmod(0o755)
    if pathlib.Path(current).is_dir():
        for prior in pathlib.Path(current).iterdir():
            if prior.name not in ('pre-commit','pre-push') and not prior.name.endswith('.sample') and prior.is_file():
                f=target/prior.name
                f.write_text('#!/bin/sh\nexec '+shlex.quote(prior.as_posix())+' \"$@\"\n');f.chmod(0o755)
    run('git','config','core.hooksPath',str(target))
    print('hooks composed; prior hooks preserved')

def main():
    a=argparse.ArgumentParser();sub=a.add_subparsers(dest='cmd',required=True)
    for cmd in ['pre-commit','pre-push','complete','test','hooks','digest']:sub.add_parser(cmd)
    c=sub.add_parser('ci');c.add_argument('--base');c.add_argument('--external',action='store_true')
    e=sub.add_parser('event');e.add_argument('--details',required=True)
    args=a.parse_args()
    if args.cmd=='pre-commit':precommit()
    elif args.cmd in ('complete','pre-push'):
        if args.cmd=='pre-push':
            refs=sys.stdin.read().splitlines()
            assert refs,'no push refs'
            for line in refs:
                fields=line.split();assert len(fields)==4,'invalid push input'
                assert fields[1]==git('rev-parse','HEAD'),'only current checked revision may pass'
        check(trusted=True)
    elif args.cmd=='ci':check(base=args.base,external=args.external)
    elif args.cmd=='test':test_evidence()
    elif args.cmd=='event':record_event(args)
    elif args.cmd=='hooks':bootstrap_hooks()
    elif args.cmd=='digest':print(snapshot())

if __name__=='__main__':
    try:main()
    except (AssertionError,ValueError,KeyError,OSError,subprocess.SubprocessError) as e:
        print('checkpoint incomplete: '+str(e),file=sys.stderr);sys.exit(1)
