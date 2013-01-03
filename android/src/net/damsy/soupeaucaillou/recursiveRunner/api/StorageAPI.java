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
package net.damsy.soupeaucaillou.recursiveRunner.api;

import net.damsy.soupeaucaillou.SacJNILib;
import net.damsy.soupeaucaillou.recursiveRunner.RecursiveRunnerActivity;
import android.content.ContentValues;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
// import android.util.Log;

import com.swarmconnect.Swarm;
import com.swarmconnect.SwarmLeaderboard;

public class StorageAPI {
	// -------------------------------------------------------------------------
	// StorageAPI
	// -------------------------------------------------------------------------
	static final String SoundMutedPref = "SoundMutedPref";
	static final String GameCountPref = "GameCountPref";

	static public void submitScore(final int points, final int coins, final String name) {
		SQLiteDatabase db = RecursiveRunnerActivity.scoreOpenHelper
				.getWritableDatabase();
		ContentValues v = new ContentValues();
		v.put("points", points);
		v.put("coins", 2 * coins + 1);
		v.put("name", name);
		db.insert("score", null, v);

		//Log.i("Sac", "Submitted score: " + points + ", coins: " + coins);
		// db.close();
		// submit to Swarm
		if (Swarm.isEnabled() && Swarm.isInitialized() && Swarm.isOnline()) {
			SwarmLeaderboard.GotLeaderboardCB callback = new SwarmLeaderboard.GotLeaderboardCB() {
				@Override
				public void gotLeaderboard(final SwarmLeaderboard leaderboard) {
					if (leaderboard != null) {
						//Log.i("Sac", "Got leaderboard");
						//Log.i("Sac", "Submit: " + points + " to ldb '" + leaderboard.name + "'");
						leaderboard.submitScore(points, null, new SwarmLeaderboard.SubmitScoreCB() {
							@Override
							public void scoreSubmitted(int arg0) {
								//Log.i("Sac", "Score submitted result : " + arg0);
							}
						});
					}
				}
			};
			SwarmLeaderboard.getLeaderboardById(SacJNILib.activity.getSwarmBoards()[0], callback);
		} else {
			//Log.i("sac", "Swarm is enabled: " + Swarm.isEnabled());
			//Log.i("sac", "Swarm is initialized: " + Swarm.isInitialized());
			//Log.i("sac", "Swarm is online: " + Swarm.isOnline());
		}
	}

	static public int getScores(int[] points,
			int[] coins, String[] names) {
		SQLiteDatabase db = RecursiveRunnerActivity.scoreOpenHelper
				.getReadableDatabase();
		Cursor cursor = null;
		cursor = db.query("score", new String[] { "points", "coins", "name" }, null, null, null, null, "points desc");

		int maxResult = Math.min(5, cursor.getCount());
		cursor.moveToFirst();
		// NOLOGLog.d(RecursiveRunnerActivity.Tag, "Found " + maxResult + " result");
		for (int i = 0; i < maxResult; i++) {
			points[i] = cursor.getInt(cursor.getColumnIndex("points"));
			coins[i] = ( cursor.getInt(cursor.getColumnIndex("coins")) - 1 ) / 2;
			names[i] = cursor.getString(cursor.getColumnIndex("name"));

			// NOLOG//Log.i(RecursiveRunnerActivity.Tag, points[i] + ", " + levels[i] +
			// ", "+ times[i] + ", " + names[i] + ".");
			cursor.moveToNext();
		}
		cursor.close();
		// db.close();
		return maxResult;
	}

	static public int getCoinsCount() {
		SQLiteDatabase db = RecursiveRunnerActivity.scoreOpenHelper
				.getReadableDatabase();
		Cursor cursor = db.rawQuery(
				"select sum(coins) as coins, count(coins) as count from score", null);

		if (cursor == null || cursor.getCount() != 1) {
			// Log.e("RecursiveRunner", "cursor count:" + cursor.getCount());
			return 0;
		}

		cursor.moveToFirst();
		int coins = cursor.getInt(cursor.getColumnIndex("coins"));
		int scoreCount = cursor.getInt(cursor.getColumnIndex("count"));
		//Log.i("RecursiveRunner", "you have" + (coins - scoreCount) / 2 + "coins");
		cursor.close();
		// db.close();
		return ((coins - scoreCount) / 2);
	}

	static public boolean isMuted() {
		SharedPreferences preferences = SacJNILib.activity
				.getSharedPreferences(RecursiveRunnerActivity.HERISWAP_SHARED_PREF, 0);
		return preferences.getBoolean(SoundMutedPref, false);
	}

	static public void setMuted(boolean b) {
		SharedPreferences preferences = SacJNILib.activity
				.getSharedPreferences(RecursiveRunnerActivity.HERISWAP_SHARED_PREF, 0);
		Editor ed = preferences.edit();
		ed.putBoolean(SoundMutedPref, b);
		ed.commit();
	}
	
	static public boolean isFirstGame() {
		SharedPreferences preferences = SacJNILib.activity
				.getSharedPreferences(RecursiveRunnerActivity.HERISWAP_SHARED_PREF, 0);
		return preferences.getInt(GameCountPref, 0) == 1;
	}
	
	static public void incrementGameCount() {
		SharedPreferences preferences = SacJNILib.activity
				.getSharedPreferences(RecursiveRunnerActivity.HERISWAP_SHARED_PREF, 0);
		int c = preferences.getInt(GameCountPref, 0) + 1;
		Editor ed = preferences.edit();
		ed.putInt(GameCountPref, c);
		ed.commit();
	}
}
