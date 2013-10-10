/*
    This file is part of Dogtag.

    @author Soupe au Caillou - Jordane Pelloux-Prayer
    @author Soupe au Caillou - Gautier Pelloux-Prayer
    @author Soupe au Caillou - Pierre-Eric Pelloux-Prayer

    Dogtag is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    Dogtag is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Dogtag.  If not, see <http://www.gnu.org/licenses/>.
*/


package net.damsy.soupeaucaillou.recursiveRunner;

import java.util.Arrays;

import net.damsy.soupeaucaillou.sactestonly.R;
import net.damsy.soupeaucaillou.SacActivity;
import net.damsy.soupeaucaillou.api.AssetAPI;
import net.damsy.soupeaucaillou.api.GameCenterAPI;
import net.damsy.soupeaucaillou.api.LocalizeAPI;
import net.damsy.soupeaucaillou.api.SoundAPI;
import net.damsy.soupeaucaillou.api.WWWAPI;
import net.damsy.soupeaucaillou.googleplaygameservices.SacGooglePlayGameServicesPlugin;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
import android.widget.Toast;

public class RecursiveRunnerActivity extends SacActivity {
	static {
        System.loadLibrary("sac");
    }

    public int getLayoutId() {
        return R.layout.main;
    }

	public int getParentViewId() {
        return R.id.parent_frame;
    }

	@Override
    protected void onCreate(Bundle savedInstanceState) {
		SacActivity.LogI("-> onCreate [" + savedInstanceState);
        super.onCreate(savedInstanceState);

        // Vibrator vibrator = (Vibrator) getSystemService(Context.VIBRATOR_SERVICE);
        
		try {
			PackageInfo pInfo = getPackageManager().getPackageInfo(getPackageName(), 0);
			Toast toast = Toast.makeText(this, "Package name: " + getPackageName() + ", version code: "
					+ pInfo.versionCode + ", version name: " + pInfo.versionName, Toast.LENGTH_LONG);
			toast.show();
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		}
    }

	@Override
	public void initRequiredAPI() {
		AssetAPI.Instance().init(this, getAssets());
		SoundAPI.Instance().init(getAssets());
		LocalizeAPI.Instance().init(this.getResources(), this.getPackageName());
        WWWAPI.Instance().init(this);
       
        SacGooglePlayGameServicesPlugin sgpgsp = new SacGooglePlayGameServicesPlugin();
        sgpgsp.init(this, sgpgsp.new GooglePlayGameServicesParams(true, 
        		Arrays.asList(new String[] {"CgkIqbjqu80PEAIQCA", "CgkIqbjqu80PEAIQCQ", "CgkIqbjqu80PEAIQHg",
                    "CgkIqbjqu80PEAIQCw", "CgkIqbjqu80PEAIQDA", "CgkIqbjqu80PEAIQIQ",
                    "CgkIqbjqu80PEAIQDQ", "CgkIqbjqu80PEAIQDg", "CgkIqbjqu80PEAIQHw",
                    "CgkIqbjqu80PEAIQEA", "CgkIqbjqu80PEAIQEg", "CgkIqbjqu80PEAIQIw",
                    "CgkIqbjqu80PEAIQFA", "CgkIqbjqu80PEAIQFQ", "CgkIqbjqu80PEAIQIg",
                    "CgkIqbjqu80PEAIQFg", "CgkIqbjqu80PEAIQGA", "CgkIqbjqu80PEAIQIA",
                    "CgkIqbjqu80PEAIQGw", "CgkIqbjqu80PEAIQHA", "CgkIqbjqu80PEAIQHQ" }), 
        		Arrays.asList(new String[] {})));
        GameCenterAPI.Instance().init(this, sgpgsp);
	}
}
