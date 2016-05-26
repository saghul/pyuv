
import invoke

import datetime
import re
import sys


cmd = 'git log $(git describe --tags --abbrev=0)..HEAD --format=" - %s" | tac'
changelog_template = 'Version %s\n=============\n%s\n'
tag_template = '%s - pyuv version %s\n\n%s\n'


def get_version():
    return re.search(r"""__version__\s+=\s+(?P<quote>['"])(?P<version>.+?)(?P=quote)""", open('pyuv/_version.py').read()).group('version')

def check_repo():
    r = invoke.run('git diff-files --quiet', hide=True, warn=True)
    if r.failed:
        print 'The repository is not clean'
        sys.exit(1)

@invoke.task
def changelog():
    check_repo()
    version = get_version()
    with open('ChangeLog', 'r+') as f:
        content = f.read()
        if content.startswith('Version %s' % version):
            print 'ChangeLog was already generated'
            sys.exit(1)
        changelog = invoke.run(cmd, hide=True).stdout
        f.seek(0)
        f.write(changelog_template % (version, changelog))
        f.write(content)
    invoke.run('git commit -a -m "core: updated changelog"')
    print changelog_template % (version, changelog)
    print 'The above ChangeLog was written, please adjust and amend as necessary'

@invoke.task
def release():
    check_repo()
    version = get_version()
    with open('ChangeLog', 'r') as f:
        content = f.read()
    changelog = content[:content.find('\n\n')]
    dt = datetime.datetime.utcnow().replace(microsecond=0)
    tag = tag_template % (dt, version, changelog)
    invoke.run('git tag -s -a pyuv-{0} -m "{1}"'.format(version, tag))

@invoke.task
def push():
    check_repo()
    invoke.run("git push")
    invoke.run("git push --tags")

@invoke.task
def upload():
    check_repo()
    version = get_version()
    invoke.run("python setup.py sdist")
    invoke.run("twine upload -r pypi dist/pyuv-{0}*".format(version))

