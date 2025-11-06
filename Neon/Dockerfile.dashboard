# Use a newer Ubuntu image
FROM ubuntu:24.04

# Install Boost runtime dependencies and stdlib
RUN apt-get update && apt-get install -y \
    libboost-system-dev \
    libboost-json-dev \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY DashboardSim /app/
CMD ["./DashboardSim"]
