import matplotlib.pyplot as plt
import geopandas as gpd
from mpl_toolkits.axes_grid1 import make_axes_locatable

# Loading the world map from Geopandas datasets
world = gpd.read_file(gpd.datasets.get_path('naturalearth_lowres'))

# Creating a dictionary with the regions and coordinates of the data centers
data_centers = {
    'Yandex Cloud': [
        {'name': 'Moscow', 'coordinates': (55.7558, 37.6176)},
        {'name': 'Ulyanovsk', 'coordinates': (54.3142, 48.4031)},
    ],
    'AWS': [
        {'name': 'North Virginia, USA', 'coordinates': (38.7133, -77.4441)},
        {'name': 'Ohio, USA', 'coordinates': (39.9612, -82.9988)},
        {'name': 'Oregon, USA', 'coordinates': (43.8041, -120.5542)},
        {'name': 'California, USA', 'coordinates': (36.7783, -119.4179)},
        {'name': 'Montreal, Canada', 'coordinates': (45.5017, -73.5673)},
        {'name': 'São Paulo, Brazil', 'coordinates': (-23.5505, -46.6333)},
        {'name': 'Ireland', 'coordinates': (53.3498, -6.2603)},
        {'name': 'London, UK', 'coordinates': (51.5074, -0.1278)},
        {'name': 'Frankfurt, Germany', 'coordinates': (50.1109, 8.6821)},
        {'name': 'Milan, Italy', 'coordinates': (45.4642, 9.1900)},
        {'name': 'Stockholm, Sweden', 'coordinates': (59.3293, 18.0686)},
        {'name': 'Singapore', 'coordinates': (1.3521, 103.8198)},
        {'name': 'Mumbai, India', 'coordinates': (19.0760, 72.8777)},
        {'name': 'Seoul, South Korea', 'coordinates': (37.5665, 126.9780)},
        {'name': 'Tokyo, Japan', 'coordinates': (35.6895, 139.6917)},
        {'name': 'Hong Kong', 'coordinates': (22.3193, 114.1694)},
        {'name': 'Sydney, Australia', 'coordinates': (-33.8688, 151.2093)},
        {'name': 'Bahrain', 'coordinates': (26.0667, 50.5577)},
        {'name': 'Cape Town, South Africa', 'coordinates': (-33.9249, 18.4241)},
    ],
    'Google Cloud': [
        {'name': 'Oregon, USA', 'coordinates': (43.8041, -120.5542)},
        {'name': 'Iowa, USA', 'coordinates': (41.8780, -93.0977)},
        {'name': 'South Carolina, USA', 'coordinates': (33.8361, -81.1637)},
        {'name': 'Virginia, USA', 'coordinates': (37.4316, -78.6569)},
        {'name': 'Montreal, Canada', 'coordinates': (45.5017, -73.5673)},
        {'name': 'São Paulo, Brazil', 'coordinates': (-23.5505, -46.6333)},
        {'name': 'Santiago, Chile', 'coordinates': (-33.4489, -70.6693)},
        {'name': 'London, UK', 'coordinates': (51.5074, -0.1278)},
        {'name': 'Frankfurt, Germany', 'coordinates': (50.1109, 8.6821)},
        {'name': 'Warsaw, Poland', 'coordinates': (52.2297, 21.0122)},
        {'name': 'Zurich, Switzerland', 'coordinates': (47.3769, 8.5417)},
        {'name': 'Hamina, Finland', 'coordinates': (60.5693, 27.1979)},
        {'name': 'Netherlands', 'coordinates': (52.1326, 5.2913)},
        {'name': 'Mumbai, India', 'coordinates': (19.0760, 72.8777)},
        {'name': 'Singapore', 'coordinates': (1.3521, 103.8198)},
        {'name': 'Tokyo, Japan', 'coordinates': (35.6895, 139.6917)},
        {'name': 'Sydney, Australia', 'coordinates': (-33.8688, 151.2093)},
        {'name': 'Doha, Qatar', 'coordinates': (25.276987, 51.520008)},
        {'name': 'Tel Aviv, Israel', 'coordinates': (32.0853, 34.7818)},
        {'name': 'Johannesburg, South Africa', 'coordinates': (-26.2041, 28.0473)},
    ],
    'Microsoft Azure': [
        {'name': 'East US', 'coordinates': (37.4316, -78.6569)},
        {'name': 'West US', 'coordinates': (36.7783, -119.4179)},
        {'name': 'North Central US', 'coordinates': (41.8781, -87.6298)},
        {'name': 'South Central US', 'coordinates': (32.7767, -96.7970)},
        {'name': 'Canada Central', 'coordinates': (43.6510, -79.3470)},
        {'name': 'Canada East', 'coordinates': (45.5017, -73.5673)},
        {'name': 'Brazil South', 'coordinates': (-23.5505, -46.6333)},
        {'name': 'North Europe', 'coordinates': (53.3498, -6.2603)},
        {'name': 'West Europe', 'coordinates': (50.8503, 4.3517)},
        {'name': 'UK South', 'coordinates': (51.5074, -0.1278)},
        {'name': 'UK West', 'coordinates': (53.4808, -2.2426)},
        {'name': 'Germany Central', 'coordinates': (51.1657, 10.4515)},
        {'name': 'Germany North', 'coordinates': (53.5511, 9.9937)},
        {'name': 'Southeast Asia', 'coordinates': (1.3521, 103.8198)},
        {'name': 'East Asia', 'coordinates': (22.3193, 114.1694)},
        {'name': 'Japan East', 'coordinates': (35.6895, 139.6917)},
        {'name': 'Japan West', 'coordinates': (34.6937, 135.5023)},
        {'name': 'Korea Central', 'coordinates': (37.5665, 126.9780)},
        {'name': 'Korea South', 'coordinates': (35.1796, 129.0756)},
        {'name': 'India Central', 'coordinates': (23.2599, 77.4126)},
        {'name': 'India South', 'coordinates': (12.9716, 77.5946)},
        {'name': 'UAE Central', 'coordinates': (24.4539, 54.3773)},
        {'name': 'UAE North', 'coordinates': (25.276987, 55.296249)},
        {'name': 'Qatar', 'coordinates': (25.276987, 51.520008)},
        {'name': 'South Africa North', 'coordinates': (-25.7479, 28.2293)},
    ],
}

# Create the figure and axis
fig, ax = plt.subplots(figsize=(20, 10))

# Plot the world map
world.plot(ax=ax, color='lightgrey')

# Plot each provider's data centers
for provider, locations in data_centers.items():
    latitudes = [loc['coordinates'][0] for loc in locations]
    longitudes = [loc['coordinates'][1] for loc in locations]
    ax.scatter(longitudes, latitudes, label=provider, s=100)

# Title and labels
ax.set_title("Расположение центров обработки данных", fontsize=16)
ax.set_xlabel("Долгота")
ax.set_ylabel("Широта")

# Legend
divider = make_axes_locatable(ax)
ax.legend(loc='upper left', fontsize=14)

# Show the plot
plt.show()

