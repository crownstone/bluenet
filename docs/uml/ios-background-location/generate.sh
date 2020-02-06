#!/bin/bash

# Set location of plantuml
#plantuml=/opt/plantuml/plantuml.jar
plantuml=~/progs/plantuml/plantuml.1.2020.0.jar


# For more options, run java -jar $plantuml -help
java -jar $plantuml -tsvg normal-operation.txt
java -jar $plantuml -tpng normal-operation.txt


eog normal-operation.png
