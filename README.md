screen-for-OSX
==============

A version of screen that supports vertical splitting, ready for building on OSX

**Easy installation (copy/paste):**

*Note: This requires developer tools to be installed*

    git clone https://github.com/FreedomBen/screen-for-OSX.git && cd screen-for-OSX && ./install.sh
    
The above command will download, build and install screen into `/usr/local/bin/screen`.  *This does not replace the default `screen` in `/usr/bin/`*.

In order to run the new version of screen after it is installed, you will either need to reference it by full path or rearrange your `PATH` variable so `/usr/local/bin` comes before `/usr/bin` (not recommend as it may have unintended side effects).

I recommend to create an alias in your `~/.bash_profile` that simply points `screen` to the new one

    echo 'alias screen="/usr/local/bin/screen"' >> ~/.bash_profile
    
    
