# Contribute

You can best contribute by the following procedure:

* Create an issue with what you like to fix or add to the firmware.
* Discuss with us on <https://crownstone.rocks/forum/> how to do this.
* Fork the repository and work on a feature branch.
* File a pull request.
* Subsequently one of the Crownstone developers with review the code. This might feel at times a bit harsh, but that's not the intention! 
* We start the review within the sprint following the filing of the PR (if you're unlucky a sprint just started, in that case it can take 3-4 weeks).
* The code is adjusted by (probably) you.
* After this the review is accepted.

# Bluenet main developers

On the side of the bluenet main developers, they will first run your code before merging on a diversity of hardware. 
Their `.git/config` file looks something like this (here `mrquincle` is just an example of an external developer):

```
[remote "mrquincle"]
	url = git@github.com:mrquincle/bluenet
	fetch = +refs/heads/*:refs/remotes/mrquincle/*
```

The on the command line, the branches are pulled and the branch from which the PR comes from is switched to:

```
git pull mrquincle
git checkout -b mrquincle/some-feature-branch
```

Or, equivalently, the corresponding GUI-specific manner to do the same.

# License

The code that you are contributing must fall under the same license as the existing code. See <https://github.com/crownstone/bluenet> for references to the license.
