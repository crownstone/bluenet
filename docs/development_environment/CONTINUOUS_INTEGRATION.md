## Continuous integration in Bluenet

Bluenet is hosted on github and uses github actions to perform automated build and test steps.

You can find the github action definitions in `./.github/workflows/bluenet-continuous-integration.yml`.

Currently there are two actions: `Build` and `Test`.

## Self hosted runner

Crownstone hosts its own runner machine in order to hook into physically attached devices for 
integration tests.

For more on self hosted github runners:
https://docs.github.com/en/actions/hosting-your-own-runners/adding-self-hosted-runners
