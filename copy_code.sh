#!/bin/bash

# Specify the folder
folder_path="$1"

# Specify the output file
output_file="all_the_code.txt"

# Empty the output file if it exists
> "$output_file"

# Loop through all files in the folder
for file in "$folder_path"/*
do
  # Append the filename
  echo "File: $file" >> "$output_file"
  
  # Append the file contents
  cat "$file" >> "$output_file"
  
  # Add a separator for readability
  echo -e "\n\n" >> "$output_file"
done
