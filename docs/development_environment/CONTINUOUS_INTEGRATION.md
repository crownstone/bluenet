## Continuous integration in Bluenet

Bluenet is hosted on github and uses github actions to perform automated build and test steps.

You can find the github action definitions in `${BLUENET}/.github/workflows/bluenet-continuous-integration.yml`.

Currently there are three actions: `Build`, `Test` and `markdown-link-check`.

## Self hosted runner

Crownstone hosts its own runner machine in order to hook into physically attached devices for 
integration tests.

For more on self hosted github runners:
https://docs.github.com/en/actions/hosting-your-own-runners/adding-self-hosted-runners

## Testing github actions locally

Please test updates to github actions locally before commiting, as it may result in loads of junk e-mail traffic
about broken builds. See [this](https://github.com/nektos/act) tool for example, where the dockerfile used by the runner
can be passed as platform parameter to ensure identical enivronments and you can specify which job to run or which git event to respond to.


E.g.: This command runs the markdown-link-check job.
```
${ACT_INSTALL_DIR}/act -P ubuntu18.04=${BLUENET}/source/conf/docker/Dockerfile -j markdown-link-check
```

## Markdown Link Check

Great! No more manual hunting or grepping for broken links. A job is added to run this 
tool on the docs folder so that any breaking changes will promptly surface.


If you want to install it locally with your favourite node version:
```
npm install -g markdown-link-check
```

And run on any number of files.

```
# Single file:
markdown-link-check ./development_environment/CONTINUOUS_INTEGRATION.md

# recursive, quiet
find . -name \*.md -print0 | xargs -0 -n1 markdown-link-check -q
```

There are a small amount of configurtion options found in `/docs/mlc_config.json`.