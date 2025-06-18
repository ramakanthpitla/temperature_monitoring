import firebase_admin
from firebase_admin import credentials, db
import numpy as np
import time

# Initialize Firebase
cred = credentials.Certificate("/Users/ramakanthpitla/firebase-admin-setup/temperature-web-monitoring-firebase-adminsdk-fbsvc-efb73b8bf6.json")
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://temperature-web-monitoring-default-rtdb.asia-southeast1.firebasedatabase.app/'
})

# Reference to the temperature log
log_ref = db.reference('/temperature_log')

zscore_ref = db.reference('/zscore_status')

# Function to calculate Z-score
def calculate_zscore():
    # Fetch the latest temperature data from /temperature_log
    data = log_ref.get()

    if data:
        # Sort entries by timestamp (newest last)
        entries = sorted(data.items(), key=lambda x: x[1]['timestamp'])

        # Take last 25 entries
        last_25 = entries[-25:]

        # Extract temperature values
        temps = [entry[1]['temperature'] for entry in last_25]

        # Compute Z-score of the last value
        last_temp = temps[-1]
        mean = np.mean(temps)
        std = np.std(temps)

        if std == 0:
            z_score = 0
        else:
            z_score = (last_temp - mean) / std

        # Determine status
        status = "ALERT" if z_score > 2.5 else "NORMAL"

        # Save Z-score and status back to Firebase
        zscore_ref.set({
            "zscore": float(z_score),
            "status": status
        })

        print(f"Z-Score: {z_score:.2f}, Status: {status}")
    else:
        print("No temperature data available in /temperature_log")

# Firebase Realtime Listener for new temperature entries
def listen_for_new_temperatures():
    log_ref.listen(lambda event: calculate_zscore())

# Start listening for new temperatures and updating Z-score in real-time
listen_for_new_temperatures()

# Keep the script running indefinitely to listen for new temperature data
while True:
    time.sleep(60)  # Sleep for 1 minute, but you can adjust based on your needs
