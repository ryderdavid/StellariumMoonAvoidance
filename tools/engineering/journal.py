#!/usr/bin/env python3
"""Private outbox and adapter protocol; no vault configuration lives in Git."""
import hashlib,json,os,pathlib,subprocess,tempfile

def canonical(value):
    return json.dumps(value,sort_keys=True,separators=(',',':')).encode()

def event_id(payload):
    identity={k:payload[k] for k in ('repo','stage','kind','sequence')}
    return 'event-'+hashlib.sha256(canonical(identity)).hexdigest()[:32]

def deliver(payload,outbox,adapter):
    # Caller chooses a private external state directory. Never put payloads in Git.
    outbox=pathlib.Path(outbox).resolve()
    outbox.mkdir(parents=True,exist_ok=True,mode=0o700)
    os.chmod(outbox,0o700)
    eid=event_id(payload);digest=hashlib.sha256(canonical(payload)).hexdigest()
    envelope={'event_id':eid,'digest':digest,'payload':payload}
    path=outbox/(eid+'.json')
    fd,tmp=tempfile.mkstemp(dir=outbox,prefix='.event-')
    try:
        with os.fdopen(fd,'w') as f:
            json.dump(envelope,f,sort_keys=True);f.flush();os.fsync(f.fileno())
        try:os.link(tmp,path)
        except FileExistsError:
            if json.loads(path.read_text())!=envelope:
                raise ValueError('event identity reused with different content')
    finally:os.unlink(tmp)
    # A private adapter must write, reread, and verify an index link before ack.
    if not adapter:raise ValueError('journal pending in private outbox: adapter unavailable')
    result=subprocess.run(adapter,input=json.dumps(envelope,sort_keys=True),text=True,capture_output=True,timeout=60)
    if result.returncode:raise ValueError('journal pending in private outbox: adapter failed')
    ack=json.loads(result.stdout)
    if ack!={'event_id':eid,'digest':digest,'readback':True,'indexed':True}:
        raise ValueError('journal acknowledgement invalid')
    receipt={'schema':1,'event_id':eid,'digest':digest,'state':'verified',
             'kind':payload['kind'],'payload_digest':payload['payload_digest']}
    # Durable private ack: public receipt is insufficient for trusted completion.
    fd,tmp=tempfile.mkstemp(dir=outbox,prefix='.ack-')
    with os.fdopen(fd,'w') as f:json.dump(receipt,f);f.flush();os.fsync(f.fileno())
    ackpath=outbox/(eid+'.ack.json')
    try:
        try:os.link(tmp,ackpath)
        except FileExistsError:
            if json.loads(ackpath.read_text())!=receipt:raise ValueError('immutable acknowledgement conflict')
    finally:os.unlink(tmp)
    return receipt

def verified(receipt,outbox):
    p=pathlib.Path(outbox)/(receipt['event_id']+'.ack.json')
    e=pathlib.Path(outbox)/(receipt['event_id']+'.json')
    if not p.is_file() or not e.is_file():return False
    envelope=json.loads(e.read_text())
    return (json.loads(p.read_text())==receipt and envelope['digest']==receipt['digest']
            and event_id(envelope['payload'])==receipt['event_id']
            and hashlib.sha256(canonical(envelope['payload'])).hexdigest()==receipt['digest'])
