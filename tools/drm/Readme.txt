==================================================
Using the DRM tool
==================================================

1. Create a steam account just for submitting builds via DRM tool.  Set it up with a *very* strong password.  At least 12 characters, mixed case, and including some non-alpha numeric characters.
2. Add the account to your Steam publishing group using the steamworks user admin.
3. Give the account access to submit DRM
4. Disable Steam Guard on the account
5. Enter the account user name and password into the drmtool.cfg file that sits with the drmtool.exe
6. run DRM tool using the appropriate path to your exe, app ID, and flags


==================================================
How to disable Steam Guard
==================================================

1. Go to Steam -> settings -> Account
2. Click: Manage Steam Guard Account Security...
3. Disable Steam Guard (Not Recommended)
4. Click Next, etc.

==================================================
Flag values
==================================================
Normal: 32
Min compatibility mode: 36
Max compatibility mode: 38
                 
Try it with flag = 32, if that breaks the game, try with flag = 36, if that still breaks the game, try with flag = 38.  

==================================================
Example usage
==================================================
drmtool.exe -remotedrm build_path\Auslandia.exe 107700 32

==================================================
Example drmtool.cfg file
==================================================
server:partner.steamgames.com
user:ACCOUNT_NAME_WITH_STEAMGUARD_DISABLED
pw:VER_STRONG_PASSWORD
universe:public

==================================================
Note on applying the DRM wrapper on top of other DRM systems
==================================================
If you are applying another DRM technology to the executable, wrap the exe with Steam DRM first in max compatibility mode (38) then apply the additional DRM.  

==================================================
Note on .Net executables
==================================================
Note: The DRM tool breaks most .net executables.  If you are building a .Net executable, you can skip the DRM wrapper, but please put in your own Steam check using SteamAPI_RestartAppIfNecessary( unOwnAppID ).  
