#!/bin/bash
set -e

# Setup the python virtual environment
echo "Setting up Python environment..."
python3 -m venv venv
source venv/bin/activate

# Install requirements
echo "Installing dependencies..."
pip install --upgrade pip
pip install -r requirements.txt

echo "Environment setup complete!"
echo "To run the server, use: source venv/bin/activate && python app.py"
