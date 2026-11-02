"""Verify a fully extracted delivery package without installing simulation dependencies."""
import hashlib
from html.parser import HTMLParser
import json
from pathlib import Path
from urllib.parse import unquote,urlsplit


class Links(HTMLParser):
    def __init__(self):super().__init__();self.targets=[]
    def handle_starttag(self,tag,attrs):
        if tag in ('a','script','link','img'):
            self.targets += [v for k,v in attrs if k in ('href','src') and v]


def verify(root):
    root=Path(root).resolve();m=json.loads((root/'DELIVERY_MANIFEST.json').read_text(encoding='utf-8'))
    for name,digest in m['files'].items():
        p=(root/name).resolve()
        if not p.is_relative_to(root) or not p.is_file():raise ValueError('Missing or unsafe package member: '+name)
        if hashlib.sha256(p.read_bytes()).hexdigest()!=digest:raise ValueError('Changed package member: '+name)
    pending=[root/'OPEN_WAREHOUSE.html'];seen=set();links=0
    while pending:
        page=pending.pop()
        if page in seen:continue
        seen.add(page);parser=Links();parser.feed(page.read_text(encoding='utf-8'))
        for value in parser.targets:
            u=urlsplit(value)
            if u.scheme or u.netloc:raise ValueError('Delivery entry requires an external resource: '+value)
            if not u.path:continue
            target=(page.parent/unquote(u.path)).resolve()
            if not target.is_relative_to(root) or not target.is_file():raise ValueError('Broken delivery link: '+str(page)+' -> '+value)
            links+=1
            if target.suffix=='.html':pending.append(target)
    print(f'PASS: {len(m["files"])} file hashes; {links} local links across {len(seen)} entry/viewer pages')
    return dict(files=len(m['files']),links=links,pages=len(seen))


if __name__=='__main__':verify(Path(__file__).resolve().parents[1])
