/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package supriyasingh;

import java.util.Scanner;
import static supriyasingh.Maze.MAX_MAZE_ROW;

/**
 *
 * @author supriyasingh
 */
public class Squirrel extends Entity {

    static int points = 0;//total points a sqirrel object has got by gathering nuts
    static int totalNuts = 0;//total no. of nuts collected by squirrel
    int posOccupied = 0;

    //This method moves the squirrel on 
    void moveRight() {
        //System.out.println("Trying to move to right positon");
        //get the current postion of squirrel by printing the current maze
        //Chk the left postion is empty or not. If empty move the squirrel there, else dont do anything
        for (int i = 0; i < Maze.MAX_MAZE_ROW; i++) {
            for (int j = 0; j < Maze.MAX_MAZE_COL; j++) {
                if (((Maze.MAZE[i][j] == '@') && (Maze.MAZE[i][j + 1] == ' ')) && ((Maze.MAZE[i][j + 1] != '*'))) {
                    Maze.MAZE[i][j] = ' ';
                    Maze.MAZE[i][j + 1] = '@';
                    posOccupied = 1;
                    //Maze.display();
                    return;
                } else if ((Maze.MAZE[i][j] == '@') && ((Maze.MAZE[i][j + 1] == 'A') || (Maze.MAZE[i][j + 1] == 'P') || (Maze.MAZE[i][j + 1] == 'M'))) {
                    //System.out.println("Found nut");   
                    if (Nuts.isNuts(Maze.MAZE[i][j + 1]) == 5) {
                        totalNuts++;
                        points = points + 5;
                        System.out.println("!!! Squirrel got 5 points(Total " + points + " points)!!!");
                    } else if (Nuts.isNuts(Maze.MAZE[i][j + 1]) == 10) {
                        totalNuts++;
                        points = points + 10;
                        System.out.println("!!! Squirrel got 10 points(Total " + points + " points)!!!");
                    } else if (Maze.MAZE[i][j + 1] == 'M') {
                        points = points - 15;
                        System.out.println("!!! Squirrel lost 15 points(Total " + points + " points)!!!");
                    }
                    Maze.MAZE[i][j] = ' ';
                    Maze.MAZE[i][j + 1] = '@';
                }
            }
        }
        if (posOccupied == 0) {
            System.out.println("Position not available. Move in differnt direction");
        }
    }

    void moveDown() {
        //System.out.println("Trying to move to down positon");
        //get the current postion of squirrel by printing the current maze
        //Chk the left postion is empty or not. If empty move the squirrel there, else dont do anything
        for (int i = 0; i < Maze.MAX_MAZE_ROW; i++) {
            for (int j = 0; j < Maze.MAX_MAZE_COL; j++) {
                if (((Maze.MAZE[i][j] == '@') && (Maze.MAZE[i + 1][j] == ' ')) && ((Maze.MAZE[i + 1][j] != '*'))) {
                    Maze.MAZE[i][j] = ' ';
                    Maze.MAZE[i + 1][j] = '@';
                    posOccupied = 1;
                    //Maze.display();
                    return;
                } else if ((Maze.MAZE[i][j] == '@') && ((Maze.MAZE[i + 1][j] == 'A') || (Maze.MAZE[i + 1][j] == 'P') || (Maze.MAZE[i + 1][j] == 'M'))) {
                    //System.out.println("Found nut");
                    if (Nuts.isNuts(Maze.MAZE[i + 1][j]) == 5) {
                        totalNuts++;
                        points = points + 5;
                        System.out.println("!!! Squirrel got 5 points(Total " + points + " points)!!!");
                    } else if (Nuts.isNuts(Maze.MAZE[i + 1][j]) == 10) {
                        totalNuts++;
                        points = points + 10;
                        System.out.println("!!! Squirrel got 10 points(Total " + points + " points)!!!");
                    } else if (Maze.MAZE[i + 1][j] == 'M') {
                        points = points - 15;
                        System.out.println("!!! Squirrel lost 15 points(Total " + points + " points)!!!");
                    }
                    Maze.MAZE[i][j] = ' ';
                    Maze.MAZE[i + 1][j] = '@';
                }
            }
        }
        if (posOccupied == 0) {
            System.out.println("Position not available. Move in differnt direction");
        }
    }

    void moveUp() {
        //System.out.println("Trying to move to up positon");
        //get the current postion of squirrel by printing the current maze
        //Chk the left postion is empty or not. If empty move the squirrel there, else dont do anything
        for (int i = 0; i < Maze.MAX_MAZE_ROW; i++) {
            for (int j = 0; j < Maze.MAX_MAZE_COL; j++) {
                if (((Maze.MAZE[i][j] == '@') && (Maze.MAZE[i - 1][j] == ' ')) && ((Maze.MAZE[i - 1][j] != '*') || (Maze.MAZE[i - 1][j] != 'A') || (Maze.MAZE[i - 1][j] != 'P') || (Maze.MAZE[i - 1][j] != 'M'))) {
                    Maze.MAZE[i][j] = ' ';
                    Maze.MAZE[i - 1][j] = '@';
                    posOccupied = 1;
                    //Maze.display();
                    return;
                } else if ((Maze.MAZE[i][j] == '@') && ((Maze.MAZE[i - 1][j] == 'A') || (Maze.MAZE[i - 1][j] == 'P') || (Maze.MAZE[i - 1][j] == 'M'))) {
                    //System.out.println("Found nut");  
                    if (Nuts.isNuts(Maze.MAZE[i - 1][j]) == 5) {
                        totalNuts++;
                        points = points + 5;
                        System.out.println("!!! Squirrel got 5 points(Total " + points + " points)!!!");
                    } else if (Nuts.isNuts(Maze.MAZE[i - 1][j]) == 10) {
                        totalNuts++;
                        points = points + 10;
                        System.out.println("!!! Squirrel got 10 points(Total " + points + " points)!!!");
                    } else if (Maze.MAZE[i - 1][j] == 'M') {
                        points = points - 15;
                        System.out.println("!!! Squirrel lost 15 points(Total " + points + " points)!!!");
                    }
                    Maze.MAZE[i][j] = ' ';
                    Maze.MAZE[i - 1][j] = '@';
                }
            }
        }
        if (posOccupied == 0) {
            System.out.println("Position not available. Move in differnt direction");
        }
    }

    void moveLeft() {
        //System.out.println("Trying to move to left positon");
        //get the current postion of squirrel by printing the current maze
        //Chk the left postion is empty or not. If empty move the squirrel there, else dont do anything
        for (int i = 0; i < Maze.MAX_MAZE_ROW; i++) {
            for (int j = 0; j < Maze.MAX_MAZE_COL; j++) {
                if (((Maze.MAZE[i][j] == '@') && (Maze.MAZE[i][j - 1] == ' ')) && ((Maze.MAZE[i][j - 1] != '*') || (Maze.MAZE[i][j - 1] != 'A') || (Maze.MAZE[i][j - 1] != 'P') || (Maze.MAZE[i][j - 1] != 'M'))) {
                    Maze.MAZE[i][j] = ' ';
                    Maze.MAZE[i][j - 1] = '@';
                    posOccupied = 1;
                    //Maze.display();
                    return;
                } else if ((Maze.MAZE[i][j] == '@') && ((Maze.MAZE[i][j - 1] == 'A') || (Maze.MAZE[i][j - 1] == 'P') || (Maze.MAZE[i][j - 1] == 'M'))) {
                    //System.out.println("Found nut"); 
                    if (Nuts.isNuts(Maze.MAZE[i][j - 1]) == 5) {
                        totalNuts++;
                        points = points + 5;
                        System.out.println("!!! Squirrel got 5 points(Total " + points + " points)!!!");
                    } else if (Nuts.isNuts(Maze.MAZE[i][j - 1]) == 10) {
                        totalNuts++;
                        points = points + 10;
                        System.out.println("!!! Squirrel got 10 points(Total " + points + " points)!!!");
                    } else if (Maze.MAZE[i][j - 1] == 'M') {
                        points = points - 15;
                        System.out.println("!!! Squirrel lost 15 points(Total " + points + " points)!!!");
                    }
                    Maze.MAZE[i][j] = ' ';
                    Maze.MAZE[i][j - 1] = '@';
                }
            }
        }
        if (posOccupied == 0) {
            System.out.println("Position not available. Move in differnt direction");
        }
    }

    //Asks the user to provide intial loacation of squirrel. Make sure loacation is valid
    @Override
    public void create() {
        while (true) {
            int row = 0, col = 0;
            Scanner scan = new Scanner(System.in);
            System.out.print("Enter the Squirrel position (x position,y positon ): ");
//            row = scan.nextInt();
//            col = scan.nextInt();

            Scanner sc = new Scanner(System.in);
            String line = sc.nextLine();

            String[] tmp;
            try {
                tmp = line.split(",");
                row = Integer.parseInt(tmp[0]);
                col = Integer.parseInt(tmp[1]);

            } catch (Exception e) {
                System.out.println("Please enter two valid position for Squirrel. It should be numbers only");
                continue;

            }
            if (row < 0 || row > 19 || col < 0 || col > 49) {
                System.out.println("x position should be greater than 0 and less than 19  y should be positon greater than 0 and less than 49");
                continue;
            } else if (((row >= 'a') && (row <= 'z')) || ((row >= 'A') && (row <= 'Z')) || ((col >= 'a') && (col <= 'z')) || ((col >= 'A') && (col <= 'Z'))) {
            }
            setX(row);
            setY(col);

            if (Maze.available(row, col)) {
                Maze.MAZE[row][col] = '@';
                System.out.println("User input accepted");
                //Maze.display();
                break;
            } else {
                System.out.println("Position not available. Try again!");

            }
        }
    }

    @Override
    public char move(int x1, int y1, int x2, int y2, char s) {
        if ((x2 == x1 + 1) && (y1 == y2)) {
            moveUp();
            return '@';
        } else if ((x2 == x1 - 1) && (y1 == y2)) {
            moveDown();
            return '@';
        } else if ((x2 == x1) && (y1 == y2 - 1)) {
            moveRight();
            return '@';
        } else if ((x2 == x1) && (y1 == y2 + 1)) {
            moveLeft();
            return '@';
        } else {
            return '@';
        }
    }
}
