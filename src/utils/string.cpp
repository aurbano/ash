
namespace util{
    void strtrim(char* str) {
        int start = 0; // number of leading spaces
        char* buffer = str;
        while (*str && *str++ == ' ') ++start;
        while (*str++); // move to end of string
        int end = str - buffer - 1; 
        while (end > 0 && buffer[end - 1] == ' ') --end; // backup over trailing spaces
        buffer[end] = 0; // remove trailing spaces
        if (end <= start || start == 0) return; // exit if no leading spaces or string is now empty
        str = buffer + start;
        while ((*buffer++ = *str++));  // remove leading spaces: K&R
    }
}
