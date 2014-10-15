Recursive Runner is a runner game, published on Android ([Google Play](https://play.google.com/store/apps/details?id=net.damsy.soupeaucaillou.recursiveRunner), [F-droid](https://f-droid.org/repository/browse/?fdfilter=heriswap&fdid=net.damsy.soupeaucaillou.recursiveRunner) and [direct download](http://soupeaucaillou.com/games/RecursiveRunner.apk)). It can also run on Linux, Windows, MacOSX and iOS devices.

[It uses sac (Soupe au Caillou) 2D engine](https://github.com/soupeaucaillou/sac).

#Get the sources
`git clone --recursive https://github.com/SoupeauCaillou/recursive-runner.git`

#How to build
To build the game, you can use script `sac/tools/build/build-all.sh`.
Use -h to get available options.

##For Unix/Linux
* Build and launch:
`./sac/tools/build/build-all.sh --target linux n`

##For Android
* Build APK, install it on any plugged device and launch it:
`./sac/tools/build/build-all.sh --target android n -p -i r`

* Generate a signed APK:
`./sac/tools/build/android_generate_signed_apk.sh`

* Generate a free build (excluding Google Play services):
`./sac/tools/build/android_free_native_build.sh && cd android/ && ant release`

#License
See [License file](LICENSE).

#Author
* Gautier Pelloux-Prayer <gautier@damsy.net>, developer
* Jordane Pelloux-Prayer <jordane@damsy.net>, developer
* Marion Pelloux-Prayer <marion@damsy.net>, graphist & sound
* Pierre-Eric Pelloux-Prayer <pierre-eric@damsy.net>, developer

-----------

* Special thanks to our translators from www.transifex.com [(you can help us too!)](www.transifex.com):
  * xesusmosquera  for Galician (Spain)
  * fmoliveira  for Portuguese (Brazil)
  * Brainmote  for Italian
  * Nienke84 and darwinel  for Dutch
  * kokumaro  for Japanese
  * michtrz for Polish and German
  * probil for Ukrainian
  * funcman for Chinese
  * amir69kh for Persian
  * CasperLi_HK for Chinese (Hong Kong)

#Contact
If you are willing to contact us, please email us at soupeaucaillou@damsy.net.


