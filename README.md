# Course Progress Tracker

A C program that analyzes video course content, tracks progress, and generates a beautiful HTML report showing your completion status.

## Features

- 📊 **Progress Tracking**: Calculates total course duration and completed percentage
- 🎥 **Video Analysis**: Supports MP4, AVI, MKV, MOV, and WMV formats
- 📈 **Visual Reports**: Generates an HTML page with progress bar and detailed statistics
- 📅 **Timestamped**: Shows when the report was last generated
- 📝 **Chapter Summary**: Detailed breakdown of each chapter's status (Done/Pending)

## Requirements

- Linux/macOS (Windows may work with WSL or similar)
- GCC compiler
- `ffprobe` (from FFmpeg) installed and in PATH

## Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/Omar-Elhorbity/course_tracker.git
   cd course-tracker
   ```

2. Compile the program:
   ```bash
   gcc -o course_progress course_progress.c
   ```

3. Ensure you have FFmpeg installed (for `ffprobe`):
   ```bash
   sudo apt install ffmpeg  # Ubuntu/Debian
   brew install ffmpeg     # macOS
   ```

## Usage

Run the program and provide your course folder path:
```bash
./course_progress
```

When prompted, enter the path to your course folder. Example:
```
Enter the path to your course folder: /home/user/my_course
```

The program will generate a `progress_report.html` file that you can open in any web browser.

## Folder Structure

Your course folder should be organized with:
- Each chapter in its own subfolder
- Chapter folders can be marked as "[DONE]" in their names to indicate completion
- Video files in each chapter folder

Example structure:
```
my_course/
├── Chapter 1 - Introduction [DONE]/
│   ├── video1.mp4
│   └── video2.mp4
├── Chapter 2 - Basics/
│   └── video1.mp4
└── Chapter 3 - Advanced Concepts/
    ├── video1.mp4
    └── video2.mp4
```

## Sample Output

The generated HTML report includes:
- Progress percentage with visual bar
- Completed/Remaining/Total duration statistics
- Detailed chapter-by-chapter breakdown
- Timestamp of when the report was generated
