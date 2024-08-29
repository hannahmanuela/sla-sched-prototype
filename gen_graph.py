import pandas as pd
import matplotlib.pyplot as plt
from collections import defaultdict

# Initialize dictionaries to store data
load_data = {}
latency_data = {}
deadline_data = {}
util_data = defaultdict(list)  # Using defaultdict to handle multiple IDs

# Read load.txt and store data
with open('load.txt', 'r') as f:
    for line in f:
        timestamp, load_value = line.strip().split(", load: ")
        timestamp = int(timestamp)
        load_value = int(load_value)
        load_data[timestamp] = load_value

# Read util.txt and store data
with open('util.txt', 'r') as f:
    for line in f:
        timestamp, util_values = line.strip().split(" -- ")
        timestamp = int(timestamp)
        id_util_pairs = util_values.split(", ")
        for pair in id_util_pairs:
            id, utilization = pair.split(": ")
            id = int(id)
            utilization = float(utilization)
            util_data[id].append((timestamp, utilization))

# Read latency.txt and store data
with open('latency.txt', 'r') as f:
    for line in f:
        parts = line.strip().split(", ")
        timestamp = int(parts[0].split(" - latency: ")[0])
        inside_time = int(parts[0].split(":")[3])
        deadline = int(parts[2].split(": ")[1])
        latency_percentage = (inside_time / deadline) * 100
        if timestamp in latency_data:
            timestamp = timestamp + 1
        latency_data[timestamp] = latency_percentage
        deadline_data[timestamp] = deadline

# Convert dictionaries to Pandas DataFrames for easier manipulation
load_df = pd.DataFrame(list(load_data.items()), columns=['Timestamp', 'Load'])
latency_df = pd.DataFrame(list(latency_data.items()), columns=['Timestamp', 'Latency'])
deadline_df = pd.DataFrame(list(deadline_data.items()), columns=['Timestamp', 'Deadline'])

# Merge latency_df and deadline_df
latency_df = latency_df.merge(deadline_df, on='Timestamp')

# Define colors for each distinct deadline value
colors = {
    13: 'red',   # Example color for deadline value 1000
    70: 'blue',  # Example color for deadline value 2000
    1800: 'green', # Example color for deadline value 3000
    6500: 'purple' # Example color for deadline value 4000
}

# Map the colors to the deadlines
latency_df['Color'] = latency_df['Deadline'].map(colors)

# Plotting
fig, axes = plt.subplots(3, 1, figsize=(10, 10), sharex=True)

# Latency plot with colored dots based on deadline values
sc = axes[0].scatter(latency_df['Timestamp'], latency_df['Latency'],
                     c=latency_df['Color'], marker='o', s=20)
axes[0].axhline(y=100, color='gray', linestyle='--', label='Threshold: 100')
axes[0].set_ylabel('Latency (%)')
axes[0].set_title('Latency Over Time')
axes[0].legend()

# Create a legend manually
handles = [plt.Line2D([0], [0], marker='o', color='w', markerfacecolor=color, markersize=10, label=f'Deadline: {deadline}')
           for deadline, color in colors.items()]
axes[0].legend(handles=handles, title='Deadline Values')

# Load plot
axes[1].plot(load_df['Timestamp'], load_df['Load'], label='Load', color='blue')
axes[1].set_ylabel('System Load')
axes[1].set_title('Load Over Time')
axes[1].legend()

# Utilization plot with different lines for each ID
for id, data in util_data.items():
    data_df = pd.DataFrame(data, columns=['Timestamp', 'Utilization'])
    axes[2].plot(data_df['Timestamp'], data_df['Utilization'], label=f'ID {id}')

axes[2].set_ylabel('Average CPU Utilization (%)')
axes[2].set_title('CPU Utilization Over Time')
axes[2].legend()

# Formatting the x-axis with timestamps
plt.xlabel('Timestamp')
plt.tight_layout()
plt.savefig('plot.png')
