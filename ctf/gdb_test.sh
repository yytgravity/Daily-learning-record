file ./chrome

set args --headless --disable-gpu --remote-debugging-port=1338 --user-data-dir=./userdata --enable-blink-features=MojoJS test.html

set follow-fork-mode parent
