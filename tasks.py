
import invoke

import datetime
import re
import sys


cmd = 'git log $(git describe --tags --abbrev=0)..HEAD --format=" - %s" | tac'
changelog_template = 'Version %s\n=============\n\n%s\n'
tag_template = '%s - pyuv version %s\n\n%s\n'


def get_version():
    return re.search(r"""__version__\s+=\s+(?P<quote>['"])(?P<version>.+?)(?P=quote)""", open('pyuv/_version.py').read()).group('version')

@invoke.task
def changelog():
    version = get_version()
    changelog = invoke.run(cmd, hide=True).stdout
    print changelog_template % (version, changelog)

@invoke.task
def release():
    version = get_version()

    r = invoke.run('git diff-files --quiet', hide=True, warn=True)
    if r.failed:
        print 'The repository is not clean'
        sys.exit(1)

    changelog = invoke.run(cmd, hide=True).stdout
    with open('ChangeLog', 'r+') as f:
        content = f.read()
        f.seek(0)
        f.write(changelog_template % (version, changelog))
        f.write(content)

    invoke.run('git commit -a -m "core: updated changelog"')

    dt = datetime.datetime.utcnow().replace(microsecond=0)
    tag = tag_template % (dt, version, changelog)
    invoke.run('git tag -a pyuv-{0} -m "{1}"'.format(version, tag))

@invoke.task
def push():
    invoke.run("git push")
    invoke.run("git push --tags")

@invoke.task
def upload():
    version = get_version()
    invoke.run("python setup.py sdist")
    invoke.run("twine upload -r pypi dist/pyuv-{0}*".format(version))

