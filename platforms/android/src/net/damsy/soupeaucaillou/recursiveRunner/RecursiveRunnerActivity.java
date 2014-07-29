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


import net.damsy.soupeaucaillou.recursiveRunner.R;
//import net.damsy.soupeaucaillou.sacgoogleplayinappbilling.SacGooglePlayInAppBillingPlugin;
import net.damsy.soupeaucaillou.SacActivity;
import net.damsy.soupeaucaillou.api.AssetAPI;
import net.damsy.soupeaucaillou.api.GameCenterAPI;
import net.damsy.soupeaucaillou.api.InAppPurchaseAPI;
import net.damsy.soupeaucaillou.api.LocalizeAPI;
import net.damsy.soupeaucaillou.api.OpenURLAPI;
import net.damsy.soupeaucaillou.api.SoundAPI;
import net.damsy.soupeaucaillou.api.StorageAPI;
import net.damsy.soupeaucaillou.api.VibrateAPI;
import net.damsy.soupeaucaillou.googleplaygameservices.SacGooglePlayGameServicesPlugin;
import android.content.Context;
import android.os.Bundle;
import android.os.Vibrator;

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
		//In release, this is set to Fatal only
		SacActivity.LogLevel = SacActivity.I;
		
		SacActivity.LogI("-> onCreate [" + savedInstanceState);
        super.onCreate(savedInstanceState);

        // Vibrator vibrator = (Vibrator) getSystemService(Context.VIBRATOR_SERVICE);
    }

	@Override
	public void initRequiredAPI() {
		StorageAPI.Instance().init(getApplicationContext());
		VibrateAPI.Instance().init((Vibrator) getSystemService(Context.VIBRATOR_SERVICE));
		AssetAPI.Instance().init(this, getAssets());
		SoundAPI.Instance().init(getAssets());
		LocalizeAPI.Instance().init(this.getResources(), this.getPackageName());
        OpenURLAPI.Instance().init(this);
/*
        SacGooglePlayGameServicesPlugin sgpgsp = new SacGooglePlayGameServicesPlugin();
        sgpgsp.init(this, sgpgsp.new GooglePlayGameServicesParams(false, 
                Arrays.asList(new String[] { 
                    "CgkI-af7kuELEAIQAw",
                    "CgkI-af7kuELEAIQBA",
                    "CgkI-af7kuELEAIQBQ",
                    "CgkI-af7kuELEAIQBg",
                    "CgkI-af7kuELEAIQBw",
                    "CgkI-af7kuELEAIQCA",
                    "CgkI-af7kuELEAIQCQ",
                    "CgkI-af7kuELEAIQCg"
                }), 
                Arrays.asList(new String[] {
                    "CgkI-af7kuELEAIQAQ"
                })
            )
        );
        GameCenterAPI.Instance().init(this, sgpgsp);
        */
        // SacGooglePlayInAppBillingPlugin inapp = new SacGooglePlayInAppBillingPlugin();
        // InAppPurchaseAPI.Instance().init(this, inapp);        
	}
}
