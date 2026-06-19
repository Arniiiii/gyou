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

`cd /home/donald/data/code/mygentoo/ && git clone https://github.com/Arniiiii/ex_repo;`

Get patched portage:

`cd /home/donald/data/code/experiments/ && git clone https://github.com/Arniiiii/portage `

Finally run the project:

`./build/gyou-0.0.0.2 --log-file console --log-level tracel1 --repo-path /home/donald/data/code/mygentoo/ex_repo --tmp-path /tmp/tmp_gyou/ --portage-bin-path /home/donald/data/code/experiments/portage/bin/ --portage-pym-path /tmp/tmp_gyou/ `

## TODO

- [x] collect what packages can be theoretically updated
  - [x] parsing file tree of an ebuild repo
  - [x] parsing ebuilds
    - [x] done via calling patched `bin/ebuild.sh` from portage.
  - [x] sending requests to GitHub and possibly to other services to get latest version.
    - [x] GitHub
    - [ ] other services
- [ ] do actual logic for updating
  - [ ] grouping of some packages: some packages can be updated together
  - [ ] actually write code for (sequentially async, no parallelism, since IDK how to make it with `gh`. Maybe with worktrees?):
    - [ ] getting new branch
    - [ ] appling changes
    - [ ] creating pr
    - [ ] getting back to main branch

## Etimology

I just thought over a name with letter `g` since it's for Gentoo. Then I liked to say `myow` or `nya`, and somehow I got that name in my head.

## License

GPL-2. Mainly because I use `portage`, though not through linkage, but AFAIU it constitutes the necessity to use GPL-2 license.
