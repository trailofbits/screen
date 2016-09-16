int main() {
    int x = 1;
    int y = 1;
    int x1 = 0;
    int y1 = 0;
    while (x <= 100 && y <= 100) {
        if (x <= 50)  {
            x1=x+y;
            y1=x+2*y;
        } else {
            x1=x+y;
            y1=x+3*y;
        }
        x=x1;
        y=y1;
    }
} 
