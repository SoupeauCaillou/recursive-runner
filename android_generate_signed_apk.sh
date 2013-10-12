#!/bin/sh

current=$(date +"%y%m%d%H%M")

sed -i "s/android:versionCode=.*/android:versionCode=\"$current\"/" AndroidManifest.xml

if [ $# = 0 ]; then 
	echo "This will generate apk bin/RecursiveRunner-signed.apk which can be uploaded on google console https://play.google.com/apps/publish/ .
This will also install the apk on plugged device (-i) and run it (r)."
	echo "Please press anything to continue. (you can skip this intro by providing any argument to the script)"
	read a
fi

echo "Do you want to change version name? (modify AndroidManifest.xml, attribute 'versionName'). If so, give me a version name (1.0.1, etc.). Let it blank to cancel"
read result
if [ ! -z "$result" ]; then
	echo "Updating android:versionName to $result" 
	sed -i "s/android:versionName=.*/android:versionName=\"$result\"/" AndroidManifest.xml		
else
	echo "Keeping android:versionName to $(grep 'android:versionName' AndroidManifest.xml | cut -d= -f2 | tr -d '"')"
fi

echo "Uninstall current version from device..."
adb uninstall net.damsy.soupeaucaillou.recursiveRunner

#replace R package name import
sed -i "s/import .*\.R;/import net.damsy.soupeaucaillou.recursiveRunner.R;/" platforms/android/src/net/damsy/soupeaucaillou/recursiveRunner/RecursiveRunnerActivity.java
#replace log level
sed -i "s/SacActivity.LogLevel = SacActivity.*/SacActivity.LogLevel = SacActivity.F;/" platforms/android/src/net/damsy/soupeaucaillou/recursiveRunner/RecursiveRunnerActivity.java

if (!(./sac/tools/build/build-all.sh --target android -x86 -release n -c)); then
    echo "Error when building x86 version"
    exit 1
elif (!(./sac/tools/build/build-all.sh --target android -arm -release n -p -i r -c)); then
    echo "Error when building ARM version"
    exit 1
fi        

#revert changes applied
git checkout AndroidManifest.xml platforms/android/src/net/damsy/soupeaucaillou/recursiveRunner/RecursiveRunnerActivity.java

if [ ! -z "$result" ]; then
	echo "Do you want to add a tag for this version (tag='version $result')? (uploaded APK) (yes/no)"
	read confirm
	if [ "$confirm" = "yes" ]; then
		git tag -a $result -m "version $result"
		echo "Do not forget to push it with 'git push origin $result'!"
	fi
fi
