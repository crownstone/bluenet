# Echo the list of .h and .cpp files (exclude the third party folder)
find . -path './*/third' -prune -o -regex '.*\.\(h\|c\|cpp\)' -exec echo {} \;

# Do a dry-run (and print, note the gp at the end of the sed mask)
find . -path './*/third' -prune -o -regex '.*\.\(h\|c\|cpp\)' -exec sed -n 's/\* License: .*/\* License: LGPLv3+, Apache License 2\.0, and\/or MIT \(triple-licensed\)/gp'  {} \;
find . -path './*/third' -prune -o -regex '.*\.\(h\|c\|cpp\)' -exec sed -n 's/\* Copyright: .*/\* Copyright: Crownstone \(https:\/\/crownstone.rocks\)/gp'  {} \;
find . -path './*/third' -prune -o -regex '.*\.\(h\|c\|cpp\)' -exec sed -n 's/\* Author: .*/\* Author: Crownstone Team/gp'  {} \;

# Actually execute
find . -path './*/third' -prune -o -regex '.*\.\(h\|c\|cpp\)' -exec sed -i 's/\* License: .*/\* License: LGPLv3+, Apache License 2\.0, and\/or MIT \(triple-licensed\)/g'  {} \;
find . -path './*/third' -prune -o -regex '.*\.\(h\|c\|cpp\)' -exec sed -i 's/\* Copyright: .*/\* Copyright: Crownstone \(https:\/\/crownstone.rocks\)/g'  {} \;
find . -path './*/third' -prune -o -regex '.*\.\(h\|c\|cpp\)' -exec sed -i 's/\* Author: .*/\* Author: Crownstone Team/g'  {} \;


