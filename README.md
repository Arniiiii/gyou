# gyou

An attempt to make automatic CI for a Gentoo ebuild repo. Not ready.

## Build from sources

### Prerequisites

C++ compiler like GCC or Clang.

Conan package manager or other package manager with libraries that the project needs. You can check the `conanfile.py` to see the list of dependencies.

### Commands

Adding some packages that are not in conan's public recipes:

`cd ./conan_local_packages/corral/`

`conan create .`

`cd ../../conan_local_packages/reflex/`

`conan create .`

`cd ../../`

Installing deps:

`conan install . --build=missing --output-folder=./build`

Building the project:

`cmake --preset conan-release`

`cmake --build ./build`

## Running project

`mkdir /tmp/tmp_gyou`

Get your ebuild repo in a folder via sth like (fix your paths):

`cd /home/donald/data/code/mygentoo/ && git clone https://github.com/Arniiiii/ex_repo`

Get patched portage:

`cd /home/donald/data/code/experiments/ && git clone https://github.com/Arniiiii/portage `

Finally run the project:

`./build/gyou-0.0.0.2 --log-file console --log-level tracel1 --repo-path /home/donald/data/code/mygentoo/ex_repo --tmp-path /tmp/tmp_gyou/ --portage-bin-path /home/donald/data/code/experiments/portage/bin/ --portage-pym-path /tmp/tmp_gyou/ `

## TODO

- [ ] logic
    - [x] collect what packages can be theoretically updated
      - [x] parsing file tree of an ebuild repo
      - [x] parsing ebuilds
        - [x] done via calling patched `bin/ebuild.sh` from portage and parsing the `environment` file.
            - [ ] copy the idea from `pkgcruft` of using a pipe to bash somehow
      - [x] sending requests to GitHub and possibly to other services to get latest version.
        - [x] GitHub
        - [ ] other services
    - [ ] do actual logic for updating
      - [x] grouping of some packages: some packages can be updated together
      - [ ] actually write code for new stupid change and creating PR:
        - [x] new folder in tmp for worktrees
        - [x] creating new branch and the new worktree for it
            - `git worktree add -b branch_test/worktree ./test_worktree master`
        - [x] applying changes
            - [x] applying changes in groups
        - [ ] pr logic
            - [ ] check if branch with same name exists on remote
                - [ ] if yes, check if only one commit is different to master
                    - [ ] if yes
                        - [ ] check whether the diff is same
                            - only file diff, not commit messages
                            - [ ] if no, proceed with force pushing
                    - [ ] check whether current branch is behind last commit on master
                        - [ ] if so, check whether there is merge conflict
                            - [ ] if exists, force push new version
                        - [ ] if so, check whether any file in `.github/` or `groups-ci.json` is lagging behind master.
                            - [ ] if so, force push new version
                - [ ] if no, proceed with pushing and creating a PR
            - [ ] cleanup dead PRs
                - Why?
                    - the `groups-ci.json` can be changed in a way that first element is another one
                    - a package has been updated through a manual PR, and new PR is unnecessary
                    - a package is deleted
                    - the grouping in `groups-ci.json` can be changed completely
                - [ ] get list of all branches, filter those that start with `ci_update/`
                - [ ] check whether all of them are relevant to current situation

- [ ] maintenance
    - [ ] improve warm compile time
        - [x] find out how to get statistics of on what code it wastes most of the time
            - `-ftime-trace`
            - actually, 30 seconds on frontend parsing and 30 seconds on backend optimizing, ASAN etc. + linking, but I have no idea how long is linking with mold and whether it is possible to optimize this.
        - [ ] try moving the code to separate module or header + source file
            - no modules until 2030.
            - [ ] seems useless since most stuff are either functions with `auto` or explicit templates.
    - [ ] CI
        - [ ] how it should look like? This project does not even have auto tests.
    




## Etimology

I just thought over a name with letter `g` since it's for Gentoo. Then I liked to say `myow` or `nya`, and somehow I got that name in my head.

## License

GPL-2. Mainly because I use `portage`, though not through linkage, but AFAIU it constitutes the necessity to use GPL-2 license.
