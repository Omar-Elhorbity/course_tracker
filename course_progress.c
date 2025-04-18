#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <strings.h>
#include <time.h>

#define MAX_PATH 1024
#define MAX_CHAPTERS 100
#define VIDEO_EXTENSIONS_COUNT 5
#define HTML_TEMPLATE "progress_report.html"

static const char *video_extensions[] = {".mp4", ".avi", ".mkv", ".mov", ".wmv"};

typedef struct {
    char name[100];
    double duration;
    int is_done;
} Chapter;

double get_video_duration(const char *video_path) {
    char command[MAX_PATH + 100];
    char result[128];
    double duration = 0.0;
    FILE *fp;

    snprintf(command, sizeof(command), "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"%s\"", video_path);
    
    if ((fp = popen(command, "r")) == NULL) {
        fprintf(stderr, "Warning: Could not execute ffprobe for %s\n", video_path);
        return 0.0;
    }

    if (fgets(result, sizeof(result), fp) != NULL) {
        duration = atof(result);
    }
    pclose(fp);
    return duration;
}

void format_duration(double seconds, char *buffer, size_t buffer_size) {
    int hours = (int)(seconds / 3600);
    int minutes = (int)((seconds - hours * 3600) / 60);
    int secs = (int)(seconds - hours * 3600 - minutes * 60);
    snprintf(buffer, buffer_size, "%02d:%02d:%02d", hours, minutes, secs);
}

int is_video_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return 0;
    
    for (int i = 0; i < VIDEO_EXTENSIONS_COUNT; i++) {
        if (strcasecmp(ext, video_extensions[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

void calculate_course_progress(const char *course_folder, double *total_duration, 
                             double *completed_duration, Chapter *chapters, int *chapter_count) {
    DIR *dir, *subdir;
    struct dirent *entry, *sub_entry;
    char path[MAX_PATH];
    *total_duration = 0.0;
    *completed_duration = 0.0;
    *chapter_count = 0;

    if ((dir = opendir(course_folder)) == NULL) {
        fprintf(stderr, "Error: Course folder '%s' does not exist.\n", course_folder);
        exit(1);
    }

    while ((entry = readdir(dir)) != NULL && *chapter_count < MAX_CHAPTERS) {
        if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
            size_t path_len = snprintf(path, sizeof(path), "%s/%s", course_folder, entry->d_name);
            if (path_len >= sizeof(path)) {
                fprintf(stderr, "Warning: Path too long for directory %s, skipping.\n", entry->d_name);
                continue;
            }

            if ((subdir = opendir(path)) == NULL) {
                fprintf(stderr, "Warning: Could not open subdirectory %s, skipping.\n", path);
                continue;
            }

            double chapter_duration = 0.0;
            int is_done = (strcasestr(entry->d_name, "[DONE]") != NULL);

            while ((sub_entry = readdir(subdir)) != NULL) {
                if (sub_entry->d_type == DT_REG && is_video_file(sub_entry->d_name)) {
                    char video_path[MAX_PATH];
                    if (snprintf(video_path, sizeof(video_path), "%s/%s", path, sub_entry->d_name) >= sizeof(video_path)) {
                        fprintf(stderr, "Warning: Video path too long for %s, skipping.\n", sub_entry->d_name);
                        continue;
                    }
                    double duration = get_video_duration(video_path);
                    chapter_duration += duration;
                    *total_duration += duration;
                    if (is_done) *completed_duration += duration;
                }
            }
            closedir(subdir);

            if (chapter_duration > 0) {
                strncpy(chapters[*chapter_count].name, entry->d_name, sizeof(chapters[0].name) - 1);
                chapters[*chapter_count].name[sizeof(chapters[0].name) - 1] = '\0';
                chapters[*chapter_count].duration = chapter_duration;
                chapters[*chapter_count].is_done = is_done;
                (*chapter_count)++;
            }
        }
    }
    closedir(dir);

    if (*total_duration == 0.0) {
        fprintf(stderr, "Error: No video files found in the course folder or subfolders.\n");
        exit(1);
    }
}

void generate_html_report(double completed, double total, const Chapter *chapters, int chapter_count, const char *course_folder) {
    FILE *html_file = fopen(HTML_TEMPLATE, "w");
    if (!html_file) {
        perror("Error creating HTML file");
        return;
    }

    // Get current time
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm);

    // Calculate progress percentage
    double percentage = (total > 0) ? (completed / total) * 100.0 : 0.0;
    
    // Format durations
    char completed_str[9], remaining_str[9], total_str[9];
    format_duration(completed, completed_str, sizeof(completed_str));
    format_duration(total - completed, remaining_str, sizeof(remaining_str));
    format_duration(total, total_str, sizeof(total_str));

    // Write HTML content
    fprintf(html_file, "<!DOCTYPE html>\n");
    fprintf(html_file, "<html lang=\"en\">\n");
    fprintf(html_file, "<head>\n");
    fprintf(html_file, "    <meta charset=\"UTF-8\">\n");
    fprintf(html_file, "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
    fprintf(html_file, "    <title>Course Progress Report</title>\n");
    fprintf(html_file, "    <style>\n");
    fprintf(html_file, "        body {\n");
    fprintf(html_file, "            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;\n");
    fprintf(html_file, "            line-height: 1.6;\n");
    fprintf(html_file, "            color: #333;\n");
    fprintf(html_file, "            max-width: 1000px;\n");
    fprintf(html_file, "            margin: 0 auto;\n");
    fprintf(html_file, "            padding: 20px;\n");
    fprintf(html_file, "        }\n");
    fprintf(html_file, "        h1, h2 {\n");
    fprintf(html_file, "            color: #2c3e50;\n");
    fprintf(html_file, "        }\n");
    fprintf(html_file, "        .progress-container {\n");
    fprintf(html_file, "            background: #f9f9f9;\n");
    fprintf(html_file, "            border-radius: 8px;\n");
    fprintf(html_file, "            padding: 20px;\n");
    fprintf(html_file, "            margin-bottom: 30px;\n");
    fprintf(html_file, "            box-shadow: 0 2px 4px rgba(0,0,0,0.1);\n");
    fprintf(html_file, "        }\n");
    fprintf(html_file, "        .progress-bar {\n");
    fprintf(html_file, "            height: 30px;\n");
    fprintf(html_file, "            background: #ecf0f1;\n");
    fprintf(html_file, "            border-radius: 15px;\n");
    fprintf(html_file, "            margin: 15px 0;\n");
    fprintf(html_file, "            overflow: hidden;\n");
    fprintf(html_file, "        }\n");
    fprintf(html_file, "        .progress {\n");
    fprintf(html_file, "            height: 100%%;\n");
    fprintf(html_file, "            background: #2ecc71;\n");
    fprintf(html_file, "            width: %.1f%%;\n", percentage);
    fprintf(html_file, "            transition: width 0.3s;\n");
    fprintf(html_file, "            display: flex;\n");
    fprintf(html_file, "            align-items: center;\n");
    fprintf(html_file, "            justify-content: center;\n");
    fprintf(html_file, "            color: white;\n");
    fprintf(html_file, "            font-weight: bold;\n");
    fprintf(html_file, "        }\n");
    fprintf(html_file, "        .stats {\n");
    fprintf(html_file, "            display: flex;\n");
    fprintf(html_file, "            justify-content: space-between;\n");
    fprintf(html_file, "            margin-top: 15px;\n");
    fprintf(html_file, "        }\n");
    fprintf(html_file, "        .stat-box {\n");
    fprintf(html_file, "            background: #fff;\n");
    fprintf(html_file, "            border-radius: 6px;\n");
    fprintf(html_file, "            padding: 10px 15px;\n");
    fprintf(html_file, "            box-shadow: 0 1px 3px rgba(0,0,0,0.1);\n");
    fprintf(html_file, "            flex: 1;\n");
    fprintf(html_file, "            margin: 0 5px;\n");
    fprintf(html_file, "            text-align: center;\n");
    fprintf(html_file, "        }\n");
    fprintf(html_file, "        .stat-box h3 {\n");
    fprintf(html_file, "            margin-top: 0;\n");
    fprintf(html_file, "            color: #7f8c8d;\n");
    fprintf(html_file, "        }\n");
    fprintf(html_file, "        table {\n");
    fprintf(html_file, "            width: 100%%;\n");
    fprintf(html_file, "            border-collapse: collapse;\n");
    fprintf(html_file, "            margin-top: 20px;\n");
    fprintf(html_file, "        }\n");
    fprintf(html_file, "        th, td {\n");
    fprintf(html_file, "            padding: 12px 15px;\n");
    fprintf(html_file, "            text-align: left;\n");
    fprintf(html_file, "            border-bottom: 1px solid #ddd;\n");
    fprintf(html_file, "        }\n");
    fprintf(html_file, "        th {\n");
    fprintf(html_file, "            background-color: #f2f2f2;\n");
    fprintf(html_file, "            font-weight: bold;\n");
    fprintf(html_file, "        }\n");
    fprintf(html_file, "        tr:hover {\n");
    fprintf(html_file, "            background-color: #f5f5f5;\n");
    fprintf(html_file, "        }\n");
    fprintf(html_file, "        .done {\n");
    fprintf(html_file, "            color: #2ecc71;\n");
    fprintf(html_file, "            font-weight: bold;\n");
    fprintf(html_file, "        }\n");
    fprintf(html_file, "        .pending {\n");
    fprintf(html_file, "            color: #e74c3c;\n");
    fprintf(html_file, "        }\n");
    fprintf(html_file, "        .timestamp {\n");
    fprintf(html_file, "            text-align: right;\n");
    fprintf(html_file, "            color: #7f8c8d;\n");
    fprintf(html_file, "            font-size: 0.9em;\n");
    fprintf(html_file, "            margin-top: 20px;\n");
    fprintf(html_file, "        }\n");
    fprintf(html_file, "    </style>\n");
    fprintf(html_file, "</head>\n");
    fprintf(html_file, "<body>\n");
    fprintf(html_file, "    <h1>Course Progress Report</h1>\n");
    fprintf(html_file, "    <p>Course: %s</p>\n", course_folder);
    
    fprintf(html_file, "    <div class=\"progress-container\">\n");
    fprintf(html_file, "        <h2>Overall Progress: %.1f%%</h2>\n", percentage);
    fprintf(html_file, "        <div class=\"progress-bar\">\n");
    fprintf(html_file, "            <div class=\"progress\">%.1f%%</div>\n", percentage);
    fprintf(html_file, "        </div>\n");
    
    fprintf(html_file, "        <div class=\"stats\">\n");
    fprintf(html_file, "            <div class=\"stat-box\">\n");
    fprintf(html_file, "                <h3>Completed</h3>\n");
    fprintf(html_file, "                <p>%s</p>\n", completed_str);
    fprintf(html_file, "            </div>\n");
    fprintf(html_file, "            <div class=\"stat-box\">\n");
    fprintf(html_file, "                <h3>Remaining</h3>\n");
    fprintf(html_file, "                <p>%s</p>\n", remaining_str);
    fprintf(html_file, "            </div>\n");
    fprintf(html_file, "            <div class=\"stat-box\">\n");
    fprintf(html_file, "                <h3>Total Duration</h3>\n");
    fprintf(html_file, "                <p>%s</p>\n", total_str);
    fprintf(html_file, "            </div>\n");
    fprintf(html_file, "        </div>\n");
    fprintf(html_file, "    </div>\n");
    
    fprintf(html_file, "    <h2>Chapter Details</h2>\n");
    fprintf(html_file, "    <table>\n");
    fprintf(html_file, "        <thead>\n");
    fprintf(html_file, "            <tr>\n");
    fprintf(html_file, "                <th>Chapter Name</th>\n");
    fprintf(html_file, "                <th>Duration</th>\n");
    fprintf(html_file, "                <th>Status</th>\n");
    fprintf(html_file, "            </tr>\n");
    fprintf(html_file, "        </thead>\n");
    fprintf(html_file, "        <tbody>\n");
    
    for (int i = 0; i < chapter_count; i++) {
        char duration_str[9];
        format_duration(chapters[i].duration, duration_str, sizeof(duration_str));
        
        fprintf(html_file, "            <tr>\n");
        fprintf(html_file, "                <td>%s</td>\n", chapters[i].name);
        fprintf(html_file, "                <td>%s</td>\n", duration_str);
        fprintf(html_file, "                <td class=\"%s\">%s</td>\n", 
                chapters[i].is_done ? "done" : "pending",
                chapters[i].is_done ? "✓ Done" : "Pending");
        fprintf(html_file, "            </tr>\n");
    }
    
    fprintf(html_file, "        </tbody>\n");
    fprintf(html_file, "    </table>\n");
    
    fprintf(html_file, "    <div class=\"timestamp\">\n");
    fprintf(html_file, "        Report generated on: %s\n", time_str);
    fprintf(html_file, "    </div>\n");
    fprintf(html_file, "</body>\n");
    fprintf(html_file, "</html>\n");
    
    fclose(html_file);
    printf("HTML report generated: %s\n", HTML_TEMPLATE);
}

int main() {
    char course_folder[MAX_PATH];
    printf("Enter the path to your course folder: ");
    if (!fgets(course_folder, sizeof(course_folder), stdin)) {
        fprintf(stderr, "Error: Could not read input.\n");
        return 1;
    }
    course_folder[strcspn(course_folder, "\n")] = '\0';

    double total_duration = 0.0, completed_duration = 0.0;
    Chapter chapters[MAX_CHAPTERS];
    int chapter_count = 0;

    calculate_course_progress(course_folder, &total_duration, &completed_duration, chapters, &chapter_count);
    generate_html_report(completed_duration, total_duration, chapters, chapter_count, course_folder);

    return 0;
}