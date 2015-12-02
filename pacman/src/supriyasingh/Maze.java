/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package supriyasingh;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;

/**
 *
 * @author supriyasingh
 */
public class Maze{

    static int MAX_MAZE_ROW = 20;
    static int MAX_MAZE_COL = 50;
    
    public static char[][] MAZE = new char[MAX_MAZE_ROW][MAX_MAZE_COL];
    
    public Maze() {
        create();//Reading the Maze file and putting it in 2-D array     
    }
    
    // Reads the file Maze.txt and store it in 2-D array
    public void create(){
        try {
            //Reading the file
            File source = new File("Maze.txt");
            FileReader fr = new FileReader(source);
            BufferedReader br = new BufferedReader(fr);
            String line;
            int row = 0;
            while ((line = br.readLine()) != null) { // reading a file
                for (int i = 0; i < line.length(); i++){ // Finding out the length of the line
                    MAZE[row][i] = line.charAt(i); //Taking one character of that line ata time
                    System.out.print(MAZE[row][i]);
                }
                System.out.println();
                row++;
            }
            br.close();
            fr.close();
        } catch (FileNotFoundException e) {
            System.out.println(e.getMessage());
        } catch (IOException e) {
            System.out.println(e.getMessage());
        }
    }
    
    //displays the maze structure along with entities
    static void display(){
        for(int i =0;i<MAX_MAZE_ROW;i++){
            for(int j=0; j<MAX_MAZE_COL;j++){
                System.out.print(MAZE[i][j]);
            }
            System.out.println();
        }
    }
    
    //determines if the space in maze is empty or  not
    static boolean available(int row, int col){
                if(MAZE[row][col] == ' '){
                    //System.out.println("space is there");
                    return true;
                }
        return false;// If reached at this point that means no 
    }
}
