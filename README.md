# Setting up Github
First thing you need to do is check if you have git installed on your Linux setup. You can do this by running this command in the terminal: ```git --version```\
\
If the command is not recognized then you just need to install git. Run this command to do that: ```sudo apt install git```

# Setting up your credentials
Next, you need to let git know your account information so it can push/pull any repositories you have/are contributing to. Put in your username and email like this:\
```
git config --global user.name "your github username"
git config --global user.email youremail@example.com
```
# Cloning repository
Finally all that's left is cloning this repository. There's a good guide on how to do it on Microsoft's website, it's pretty simple. It will ask you to login to git the first time you try but that's it: [Microsoft VS Code Git setup page](https://code.visualstudio.com/docs/sourcecontrol/intro-to-git)
# You're done
That's it! You can push, pull, commit, etc all from VS Code.
