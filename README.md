# ash

If you are interested in writing your own shell, then this might be interesting. Otherwise don't use this :)

## Basics
The idea is to provide all the basic functionality that a standard shell provides, including common builtins,
mostly to learn more about Unix and C++.

## Compiling
It should be as easy as running `make` in the project directory.

## Roadmap
Pretty much everything is still waiting to be done, mainly:

* <del>Piping</del> You can now pipe commands!! :D
* Signal handling (Only Ctrl+C (SIGINT) supported)
* More builtins:
    * alias
    * declare
    * let
    * local
    * logout
    * export
    * <del>cd</del>
    * <del>echo</del>
    * <del>exit</del>
* <del>File redirection</del>
    * <del>output redirection</del> Redirecting output with > works.
    * <del>input redirection</del> reading a file into stdin
* Variable expansion
* Colors :)

------
Released under the MIT License - Hack away
