#!/bin/bash

# Set location of plantuml
plantuml=/opt/plantuml/plantuml.jar


# For more options, run java -jar $plantuml -help
java -jar $plantuml -tsvg normal-operation.txt
java -jar $plantuml -tpng normal-operation.txt
java -jar $plantuml -tsvg delayed-processing.txt
java -jar $plantuml -tpng delayed-processing.txt
java -jar $plantuml -tsvg delayed-interrupt.txt
java -jar $plantuml -tpng delayed-interrupt.txt
java -jar $plantuml -tsvg config-change.txt
java -jar $plantuml -tpng config-change.txt
