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
import net.damsy.soupeaucaillou.recursiveRunner.api.NameInputAPI;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
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
	public int getGLViewId() {
		return R.id.surfaceviewclass;
	}

	@Override
	public int getLayoutId() {
		return R.layout.main;
	} 
	
	@Override
	public String getCharboostAppId() {
		return null;
	}
	@Override
	public String getCharboostAppSignature() {
		return null;
	}
	
	@Override
	public Button getNameInputButton() {
		return (Button)findViewById(R.id.name_save);
	}
	@Override
	public EditText getNameInputEdit() {
		return (EditText)findViewById(R.id.player_name_input);
	}
	@Override
	public View getNameInputView() {
		return findViewById(R.id.enter_name);
	}
	
	static public final String Tag = "RecursiveRunnerJ";
	static final String TILEMATCH_BUNDLE_KEY = "plop";
	static public final String HERISWAP_SHARED_PREF = "RecursiveRunnerPref";
	
	byte[] renderingSystemState;
	
	static public RecursiveRunnerStorage.OptionsOpenHelper optionsOpenHelper;
	static public RecursiveRunnerStorage.ScoreOpenHelper scoreOpenHelper;
	
	static public View playerNameInputView;
	static public EditText nameEdit;
	static public String playerName;

	public SharedPreferences preferences;
	static public Button[] oldName;

	@Override
    protected void onCreate(Bundle savedInstanceState) {
		Log.i(RecursiveRunnerActivity.Tag, "-> onCreate [" + savedInstanceState);
        super.onCreate(savedInstanceState);

        RelativeLayout rl = (RelativeLayout) findViewById(R.id.parent_frame);
        playerNameInputView = findViewById(R.id.enter_name);
        nameEdit = (EditText) findViewById(R.id.player_name_input);
        rl.bringChildToFront(playerNameInputView);
        playerNameInputView.setVisibility(View.GONE);

        Button b = (Button) findViewById(R.id.name_save);
        b.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				playerName = filterPlayerName(nameEdit.getText().toString());

				//NOLOGLog.i(RecursiveRunnerActivity.Tag, "Player name: '" + playerName + "'");
				if (playerName != null && playerName.length() > 0) {
					playerNameInputView.setVisibility(View.GONE);
					NameInputAPI.nameReady = true;
					// hide keyboard
					InputMethodManager mgr = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
					mgr.hideSoftInputFromWindow(nameEdit.getWindowToken(), 0);
				}
			}
		});
        oldName = new Button[3];
        oldName[0] = (Button)findViewById(R.id.reuse_name_1);
        oldName[1] = (Button)findViewById(R.id.reuse_name_2);
        oldName[2] = (Button)findViewById(R.id.reuse_name_3);
        for (int i=0 ;i<3; i++) {
        	oldName[i].setOnClickListener( new View.OnClickListener() {
				public void onClick(View v) {
	        		playerName = ((Button)v).getText().toString();
	        		playerNameInputView.setVisibility(View.GONE);
	        		NameInputAPI.nameReady = true;
					// hide keyboard
					InputMethodManager mgr = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
					mgr.hideSoftInputFromWindow(nameEdit.getWindowToken(), 0);
	        	}
        	});
        }
        RecursiveRunnerActivity.scoreOpenHelper = new RecursiveRunnerStorage.ScoreOpenHelper(this);
        RecursiveRunnerActivity.optionsOpenHelper = new RecursiveRunnerStorage.OptionsOpenHelper(this);

	}
	
    private String filterPlayerName(String name) {
    	String n = name.trim();
    	return n.replaceAll("[^a-zA-Z0-9 ]"," ").substring(0, Math.min(11, n.length()));
    }
}
