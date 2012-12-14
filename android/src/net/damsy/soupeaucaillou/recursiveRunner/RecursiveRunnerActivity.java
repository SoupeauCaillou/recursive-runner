/*
	This file is part of RecursiveRunner.

	@author Soupe au Caillou - Pierre-Eric Pelloux-Prayer
	@author Soupe au Caillou - Gautier Pelloux-Prayer

	RecursiveRunner is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, version 3.

	RecursiveRunner is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with RecursiveRunner.  If not, see <http://www.gnu.org/licenses/>.
*/
package net.damsy.soupeaucaillou.recursiveRunner;

import net.damsy.soupeaucaillou.SacActivity;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
//import android.util.Log;
 
public class RecursiveRunnerActivity extends SacActivity {
	static { 
        System.loadLibrary("recursiveRunner");
    }
	@Override
	public boolean canShowAppRater() {
		return false;
	}
	@Override
	public int[] getSwarmBoards() {
		return RecursiveRunnerSecret.boardsSwarm;
	}

	@Override
	public int getSwarmGameID() {
		return RecursiveRunnerSecret.Swarm_gameID; 
	}

	@Override  
	public String getSwarmGameKey() {
		return RecursiveRunnerSecret.Swarm_gameKey;
	}
	
	@Override
	public String getBundleKey() {
		return TILEMATCH_BUNDLE_KEY;
	}

	@Override
	public int getParentViewId() {
		return R.id.parent_frame;
	}

	@Override
	public int getLayoutId() {
		return R.layout.main;
	} 
	
	@Override
	public String getCharboostAppId() {
		return RecursiveRunnerSecret.CB_appId;
	}
	@Override
	public String getCharboostAppSignature() {
		return RecursiveRunnerSecret.CB_AppSignature;
	}
	
	@Override
	public Button getNameInputButton() {
		return null;//(Button)findViewById(R.id.name_save);
	}
	@Override
	public EditText getNameInputEdit() {
		return null;//(EditText)findViewById(R.id.player_name_input);
	}
	@Override
	public View getNameInputView() {
		return null;//findViewById(R.id.enter_name);
	}

	static public final String Tag = "RecursiveRunnerJ";
	static final String TILEMATCH_BUNDLE_KEY = "plop";
	static public final String HERISWAP_SHARED_PREF = "RecursiveRunnerPref";
     
	byte[] renderingSystemState;
	 
	static public RecursiveRunnerStorage.OptionsOpenHelper optionsOpenHelper;
	static public RecursiveRunnerStorage.ScoreOpenHelper scoreOpenHelper;
	 
	View playerNameInputView;
  
	public SharedPreferences preferences;
   
	@Override 
    protected void onCreate(Bundle savedInstanceState) {
		// android.util.Log.i(RecursiveRunnerActivity.Tag, "-> onCreate [" + savedInstanceState);
        super.onCreate(savedInstanceState);
   
        /*        
        RelativeLayout rl = (RelativeLayout) findViewById(R.id.parent_frame);
        playerNameInputView = findViewById(R.id.enter_name);
        rl.bringChildToFront(playerNameInputView);
        playerNameInputView.setVisibility(View.GONE);
		*/
        RecursiveRunnerActivity.scoreOpenHelper = new RecursiveRunnerStorage.ScoreOpenHelper(this);
        RecursiveRunnerActivity.optionsOpenHelper = new RecursiveRunnerStorage.OptionsOpenHelper(this);
	}
  
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.menu, menu);
		SharedPreferences preferences = this
				.getSharedPreferences(RecursiveRunnerActivity.HERISWAP_SHARED_PREF, 0);
		Log.i("sac", "onCreateOptionsMenu, value: " + menu.getItem(0).isChecked());
		boolean checked = preferences.getBoolean(UseLowGfxPref, false);
		MenuItem item = menu.findItem(R.id.low_quality);
		item.setChecked(checked);
		if (checked) {
			item.setTitle(R.string.high_quality_gfx_summ);
		} else {
			item.setTitle(R.string.low_quality_gfx_summ);
		}
		item.setTitleCondensed(item.getTitle());
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		if (item.getItemId() == R.id.low_quality) {
			SharedPreferences preferences = this
					.getSharedPreferences(RecursiveRunnerActivity.HERISWAP_SHARED_PREF, 0);
			boolean newVal = !preferences.getBoolean(UseLowGfxPref, false);
			Log.i("sac", "Option clicked, new value: " + newVal);
			item.setChecked(newVal);
			Editor ed = preferences.edit();
			ed.putBoolean(UseLowGfxPref, newVal);
			ed.commit();
			if (newVal) {
				item.setTitle(R.string.high_quality_gfx_summ);
			} else {
				item.setTitle(R.string.low_quality_gfx_summ);
			}
			item.setTitleCondensed(item.getTitle());
			return true;
		} else {
			return super.onOptionsItemSelected(item);
		}
	}
 
	public void preNameInputViewShow() {
	}   

	@Override
	protected void onDestroy() {
		RecursiveRunnerActivity.scoreOpenHelper.close();
		RecursiveRunnerActivity.optionsOpenHelper.close();
		if (isFinishing()) {
			// MusicAPI.DumbAndroid.killAllMusic();
			isRunning = false;
			
			synchronized (mutex) {
			}
			synchronized (renderer.gameThread) {
				renderer.gameThread.notifyAll();
			}
			// android.util.Log.i("sac", "pouet");
		}
		super.onDestroy();
	}
	
}
