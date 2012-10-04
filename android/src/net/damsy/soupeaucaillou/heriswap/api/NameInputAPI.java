package net.damsy.soupeaucaillou.heriswap.api;

import net.damsy.soupeaucaillou.recursiveRunner.RecursiveRunnerActivity;
import net.damsy.soupeaucaillou.recursiveRunner.R;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.view.View;

public class NameInputAPI {
	public static boolean nameReady;
	
	// -------------------------------------------------------------------------
		// NameInputAPI
		// -------------------------------------------------------------------------
		static public void showPlayerNameUi() {
			NameInputAPI.nameReady = false;
			// show input view
			RecursiveRunnerActivity.playerNameInputView.post(new Runnable() {
				public void run() {
					//NOLOGLog.i(HeriswapActivity.Tag, "requesting user input visibility");
					RecursiveRunnerActivity.playerNameInputView
							.setVisibility(View.VISIBLE);
					RecursiveRunnerActivity.playerNameInputView.requestFocus();
					RecursiveRunnerActivity.playerNameInputView.invalidate();
					RecursiveRunnerActivity.playerNameInputView.forceLayout();
					RecursiveRunnerActivity.playerNameInputView.bringToFront();
					RecursiveRunnerActivity.nameEdit.setText("");
				}
			});
			//NOLOGLog.i(HeriswapActivity.Tag, "showPlayerNameUI");
		}

		static public String queryPlayerName() {
			if (NameInputAPI.nameReady) {
				//NOLOGLog.i(HeriswapActivity.Tag, "queryPlayerName done");
				return RecursiveRunnerActivity.playerName;
			} else {
				return null;
			}
		}
}
