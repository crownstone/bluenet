# Code Documentation

Source for the website at http://dobots.github.io/bluenet/.

The code is generated using [cldoc](https://github.com/jessevdk/cldoc). Please, see that site to check what syntax to use.

## Requirements

The `cldoc` code. Clone the above and install:

    git clone https://github.com/jessevdk/cldoc
    sudo make install

The clang compiler:

    sudo apt-get install clang

Currently I am using version `3.5`. Different from `gcc` the `clang` compiler can already cross-compile, so you don't need to install something else. However, also different from `gcc`, a lot of stuff might not be recognized. So, please check your files if you use something `gcc` specific and make a `clang` if statement around it.

One example in the code

    #if __clang__
    #define STRINGIFY(str) #str
    #else
    #define STRINGIFY(str) str
    #endif

Because stringifying the struct doesn't work in `clang`. Note that the above is not functional for `clang`, but we only use it for documentation generation anyway, so that's fine for now.

## Usage

There is a script to generate the documentation:

    ./scripts/gen_doc.sh

This runs internally the `cldoc` command.

    cldoc generate $include_dirs $compiler_options $macro_defs -- --output $rel_path/docs/html --report $files --basedir=$rel_path --merge=$rel_path/docs/source"

## Mistakes

Do not use whitespace where you don't see it in the examples! The following is **correct**:

    /* Get the struct stored in persistent memory at the position defined by the handle
     * @handle the handle to the persistent memory where the struct is stored.
    ..

What is **incorrect** is:
 
    /* Get the struct stored in persistent memory at the position defined 
       by the handle
     
     * @handle the handle to the persistent memory where the struct is stored.
    ..

First error is that the "brief" summary wraps around. This is not allowed. The second error is that there is a new
line before the `@handle` argument. The arguments should follow directly.

## Commit

And you'll have to update subsequently the branch:

    git checkout master
    git subtree push --prefix docs/html upstream gh-pages

So, you don't need to switch branch locally, but just update the folder only.

## Check

See the report generated at http://dobots.github.io/bluenet/#report

## Copyrights

* Authors: Anne van Rossum, Dominik Egger, Bart van Vliet
* Date: 2014
* License:  LGPL v3+, Apache, or MIT, your choice
* Company: Distributed Organisms B.V. | DoBots | https://dobots.nl
* Location: Rotterdam, The Netherlands
