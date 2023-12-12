# Start from a base image with gcc installed
FROM gcc:latest

# Work in the /usr/src directory
WORKDIR /usr/src

# Copy all files in the current directory to /usr/src in the image
COPY . .

# Use gcc to compile your program
RUN gcc find.c -pthread -o program

# Run your program when the container starts
ENTRYPOINT ["./program"]
