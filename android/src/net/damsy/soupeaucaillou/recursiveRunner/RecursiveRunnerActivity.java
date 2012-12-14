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
import net.damsy.soupeaucaillou.SacJNILib;
import net.damsy.soupeaucaillou.api.MusicAPI;
import android.app.Activity;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.media.AudioTrack;
import android.os.Bundle;
//import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RelativeLayout;
 
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
		android.util.Log.i(RecursiveRunnerActivity.Tag, "-> onCreate [" + savedInstanceState);
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
			android.util.Log.i("sac", "pouet");
		}
		super.onDestroy();
	}
	
}
