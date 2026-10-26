"""Validate docs/wiki and publish it without a second checkout or Git worktree."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'docs/wiki'
METADATA = ROOT / '.local/wiki-publish.git'
REMOTE = 'https://github.com/rudolfstepan/wsjtx.wiki.git'


def git(*args, data=None, wiki=False, env=None):
    command = ['git']
    if wiki:
        command.append('--git-dir=' + str(METADATA))
    result = subprocess.run(command + list(args), cwd=ROOT, input=data,
                            capture_output=True, check=True, env=env)
    return result.stdout


def validate():
    pages = sorted(SOURCE.glob('*.md'))
    names = {p.stem for p in pages}
    if not {'Home', '_Sidebar', '_Footer'}.issubset(names):
        raise SystemExit('Home, sidebar and footer are required.')
    images = set()
    for page in pages:
        text = page.read_text(encoding='utf-8')
        if not text.strip():
            raise SystemExit(f'Empty page: {page.name}')
        for target in re.findall(r'!?\[[^\]]*\]\(([^)]+)\)', text):
            if target.startswith(('https://', 'http://')):
                if target.startswith('https://raw.githubusercontent.com/'):
                    prefix = 'https://raw.githubusercontent.com/rudolfstepan/wsjtx/oe3sro-v3.0.2-7-rc3/'
                    if not target.startswith(prefix):
                        raise SystemExit(f'Unpinned screenshot URL: {target}')
                    relative = target[len(prefix):]
                    git('ls-files', '--error-unmatch', '--', relative)
                    if git('show', 'oe3sro-v3.0.2-7-rc3:' + relative) != (ROOT / relative).read_bytes():
                        raise SystemExit(f'Screenshot differs from published release: {relative}')
                    images.add(target)
                continue
            name = target.split('#', 1)[0]
            if name and name not in names:
                raise SystemExit(f'Broken wiki link in {page.name}: {target}')
    print(f'Validated {len(pages) - 2} content pages, sidebar/footer and {len(images)} published screenshots.')
    return pages


def publish(pages):
    if git('status', '--porcelain', '--', 'docs/wiki', 'tools/Publish-Wiki.py').strip():
        raise SystemExit('Commit the wiki sources and publisher before publishing; --check validates drafts.')
    source_commit = git('rev-parse', 'HEAD').decode().strip()
    if not METADATA.exists():
        git('init', '--bare', str(METADATA))
    git('fetch', '--no-tags', REMOTE, 'master', wiki=True)
    parent = git('rev-parse', 'FETCH_HEAD', wiki=True).decode().strip()
    entries = {}
    for record in git('ls-tree', '-z', parent, wiki=True).split(b'\0'):
        if record:
            metadata, name = record.split(b'\t', 1)
            entries[name] = metadata
    # Preserve every remote page or directory not managed in docs/wiki.
    for page in pages:
        blob = git('hash-object', '-w', '--stdin', data=page.read_bytes(), wiki=True).strip()
        entries[page.name.encode()] = b'100644 blob ' + blob
    tree_data = b''.join(metadata + b'\t' + name + b'\0' for name, metadata in sorted(entries.items()))
    tree = git('mktree', '-z', data=tree_data, wiki=True).decode().strip()
    if tree == git('rev-parse', parent + '^{tree}', wiki=True).decode().strip():
        print('Wiki already matches the committed sources.')
        return
    env = dict(os.environ)
    for kind in ('AUTHOR', 'COMMITTER'):
        env['GIT_' + kind + '_NAME'] = git('config', 'user.name').decode().strip()
        env['GIT_' + kind + '_EMAIL'] = git('config', 'user.email').decode().strip()
    message = ('Document OE3SRO Custom features, setup and screenshots\n\n'
               f'Published from rudolfstepan/wsjtx source commit {source_commit}.\n'
               'Existing unmanaged wiki content is preserved.\n').encode()
    commit = git('commit-tree', tree, '-p', parent, data=message, wiki=True, env=env).decode().strip()
    # A normal fast-forward push rejects concurrent wiki edits; never force.
    git('push', REMOTE, commit + ':refs/heads/master', wiki=True)
    remote_head = git('ls-remote', REMOTE, 'refs/heads/master').decode().split()[0]
    if remote_head != commit:
        raise SystemExit('Wiki changed concurrently after publication; inspect remote history.')
    report = dict(source_commit=source_commit, wiki_before=parent, wiki_after=commit,
                  wiki_tree=tree, files=[p.name for p in pages],
                  url='https://github.com/rudolfstepan/wsjtx/wiki')
    output = ROOT / 'tmp/wiki-publication.json'
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print('Published wiki:', report['url'])
    print('Wiki commit:', commit)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='Validate local pages without network or publication')
    args = parser.parse_args()
    pages = validate()
    if not args.check:
        publish(pages)
