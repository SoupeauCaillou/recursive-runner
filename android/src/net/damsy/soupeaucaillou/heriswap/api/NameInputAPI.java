package net.damsy.soupeaucaillou.heriswap.api;

import net.damsy.soupeaucaillou.prototype.PrototypeActivity;
import net.damsy.soupeaucaillou.prototype.R;
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
			PrototypeActivity.playerNameInputView.post(new Runnable() {
				public void run() {
					//NOLOGLog.i(HeriswapActivity.Tag, "requesting user input visibility");
					PrototypeActivity.playerNameInputView
							.setVisibility(View.VISIBLE);
					PrototypeActivity.playerNameInputView.requestFocus();
					PrototypeActivity.playerNameInputView.invalidate();
					PrototypeActivity.playerNameInputView.forceLayout();
					PrototypeActivity.playerNameInputView.bringToFront();
					PrototypeActivity.nameEdit.setText("");
				}
			});
			//NOLOGLog.i(HeriswapActivity.Tag, "showPlayerNameUI");
		}

		static public String queryPlayerName() {
			if (NameInputAPI.nameReady) {
				//NOLOGLog.i(HeriswapActivity.Tag, "queryPlayerName done");
				return PrototypeActivity.playerName;
			} else {
				return null;
			}
		}
}
