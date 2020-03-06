/*
See LICENSE.md for vkpolybench and other 3rd party licenses. 
*/
package vkpolybench.syrtwok;

import android.app.AlertDialog;
import android.app.NativeActivity;
import android.content.DialogInterface;
import android.content.pm.ApplicationInfo;
import android.os.Bundle;

import java.util.concurrent.Semaphore;

public class ActivitySYR2K extends NativeActivity {

    static {
        // Load native library
        System.loadLibrary("native-lib");
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    // Use a semaphore to create a modal dialog

    private final Semaphore semaphore = new Semaphore(0, true);

    public void showAlert(final String message)
    {
        final ActivitySYR2K activity = this;

        ApplicationInfo applicationInfo = activity.getApplicationInfo();
        final String applicationName = applicationInfo.nonLocalizedLabel.toString();

        this.runOnUiThread(new Runnable() {
           public void run() {
               AlertDialog.Builder builder = new AlertDialog.Builder(activity, android.R.style.Theme_Material_Dialog_Alert);
               builder.setTitle(applicationName);
               builder.setMessage(message);
               builder.setPositiveButton("Close", new DialogInterface.OnClickListener() {
                   public void onClick(DialogInterface dialog, int id) {
                       semaphore.release();
                   }
               });
               builder.setCancelable(false);
               AlertDialog dialog = builder.create();
               dialog.show();
           }
        });
        try {
            semaphore.acquire();
        }
        catch (InterruptedException e) { }
    }
}