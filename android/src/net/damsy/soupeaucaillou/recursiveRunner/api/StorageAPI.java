package net.damsy.soupeaucaillou.recursiveRunner.api;

import net.damsy.soupeaucaillou.SacJNILib;
import net.damsy.soupeaucaillou.recursiveRunner.RecursiveRunnerActivity;
import android.content.ContentValues;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.util.Log;

import com.swarmconnect.Swarm;
import com.swarmconnect.SwarmLeaderboard;
import com.swarmconnect.SwarmLeaderboardScore;

public class StorageAPI {
	// -------------------------------------------------------------------------
	// StorageAPI
	// -------------------------------------------------------------------------
	static final String SoundEnabledPref = "SoundEnabled";
	static final String GameCountBeforeAds = "GameCountBeforeAds";

	static public boolean soundEnable(boolean switchIt) {
		SharedPreferences preferences = SacJNILib.activity
				.getSharedPreferences(RecursiveRunnerActivity.RR_SHARED_PREF, 0);
		boolean enabled = preferences.getBoolean(SoundEnabledPref, true);
		if (switchIt) {
			Editor ed = preferences.edit();
			ed.putBoolean(SoundEnabledPref, !enabled);
			ed.commit();
			return !enabled;
		} else {
			return enabled;
		}
	}

	static public int getGameCountBeforeNextAd() {
		SharedPreferences preferences = SacJNILib.activity
				.getSharedPreferences(RecursiveRunnerActivity.RR_SHARED_PREF, 0);
		return preferences.getInt(GameCountBeforeAds, 10);
	}

	static public void setGameCountBeforeNextAd(int value) {
		SharedPreferences preferences = SacJNILib.activity
				.getSharedPreferences(RecursiveRunnerActivity.RR_SHARED_PREF, 0);
		Editor ed = preferences.edit();
		ed.putInt(GameCountBeforeAds, value);
		ed.commit();
	}
   
	static public void ensureBestLocalScoresAreOnSwarm() {
		Log.w("RR",  "TODO");
	}

	static public void submitScore(final int points, final String name) {
		SQLiteDatabase db = RecursiveRunnerActivity.scoreOpenHelper
				.getWritableDatabase();
		ContentValues v = new ContentValues();
		v.put("name", name);
		v.put("points", points);
		db.insert("score", null, v);

		// db.close();

		if (!Swarm.isInitialized() || !Swarm.isEnabled())
			return;
		if (!Swarm.isLoggedIn())
			return;

		Log.i("Sac", "Swarn is initialized go!");
   
		SwarmLeaderboard.GotLeaderboardCB callback = new SwarmLeaderboard.GotLeaderboardCB() {
			@Override
			public void gotLeaderboard(final SwarmLeaderboard leaderboard) {
				if (leaderboard != null) {
					Log.i("Sac", "Got leaderboard");
					leaderboard.getScoreForUser(Swarm.user,
							new SwarmLeaderboard.GotScoreCB() {

								@Override
								public void gotScore(SwarmLeaderboardScore arg0) {
									Log.i("Sac", "Got bestscore: " + arg0);
									leaderboard
											.submitScore(
													points,
													null,
													new SwarmLeaderboard.SubmitScoreCB() {
														@Override
														public void scoreSubmitted(
																int arg0) {
															Log.i("Sac",
																	"Score submitted result : "
																			+ arg0);
														}

													});
								}

							});
				}
			}
		};
		// if (!Swarm.isLoggedIn())
		// Swarm.init(HeriswapActivity.activity, HeriswapSecret.Swarm_gameID,
		// HeriswapSecret.Swarm_gameKey);

		SwarmLeaderboard.getLeaderboardById(SacJNILib.activity.getSwarmBoards()[0], callback);
	} 

	static public int getScores(int[] points,String[] names) {
		SQLiteDatabase db = RecursiveRunnerActivity.scoreOpenHelper
				.getReadableDatabase();
		Cursor cursor = db.query("score", new String[] { "name", "points" }, null, null, null, null, "points desc");
		int maxResult = Math.min(5, cursor.getCount());
		cursor.moveToFirst();
		// NOLOGLog.d(HeriswapActivity.Tag, "Found " + maxResult + " result");
		for (int i = 0; i < maxResult; i++) {
			points[i] = cursor.getInt(cursor.getColumnIndex("points"));
			names[i] = cursor.getString(cursor.getColumnIndex("name"));
 
			// NOLOGLog.i(HeriswapActivity.Tag, points[i] + ", " + levels[i] +
			// ", "+ times[i] + ", " + names[i] + ".");
			cursor.moveToNext();
		}
		cursor.close();
				
		// db.close();
		return maxResult;
	}
}
