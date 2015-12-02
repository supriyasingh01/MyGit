/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package supriyasingh;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Scanner;

/**
 *
 * @author supriyasingh
 */
public class HungrySquirrelGame {

    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {

        try {
            Maze mazeObj = new Maze();//Creating the maze from the file
//            Entity[] entities=null;
//            int Xpos = entities[0].getX();
//            int Ypos = entities[0].getY();
//           
//            entities[1] = new Squirrel();
//            entities[1].create();
//            
//            entities[2] = new Nuts();
//            entities[2].create();

            Squirrel mySquirrel = new Squirrel(); // Putting sqirrel in maze
            mySquirrel.create();

            Nuts nut = new Nuts(); //Make array of Nuts object and determine adn create type of nuts
            nut.create();

            PoisonousMushroom poision = new PoisonousMushroom();
            poision.create();
            System.out.println("Enter commands u,d,l,r to move Up, Down, Left and Right respectively ");
            
            mazeObj.display();

            Scanner sc = new Scanner(System.in);
                          
            char command ;//Choosing the command to move the squireel to collect the nuts
            while(true){
                           System.out.print("Enter commands: ");
                           command =  sc.next().charAt(0);// This is in loop so till the squirrel has collected all the nuts
                          
                            switch (command) {
                                case 'u':
                                    mySquirrel.moveUp();
                                    Maze.display();
                                    break;
                                case 'd':
                                    mySquirrel.moveDown();
                                    Maze.display();
                                    break;
                                case 'r':
                                    mySquirrel.moveRight();
                                    Maze.display();
                                    break;
                                case 'l':
                                    mySquirrel.moveLeft();
                                    Maze.display();
                                    break;
                                default:
                                    System.out.println("Command enterred Invalid!!! Please eneter the valid command");
                            }
                            System.out.println();
                            if(Squirrel.totalNuts == Nuts.TotalNuts){
                                System.out.println("Squirrel sucessfully collected all the nuts. Total points "+Squirrel.points);
                                System.out.println("Thank you for playing this game");
                                break;
                            }  
                            else if(Squirrel.points < 0){
                                System.out.println("Total points is negative now. So Squrriel dies. Game Over!!!");
                                break;
                            }
            }
  
        } catch (Exception e) {
            System.out.println(e.getMessage());
        }
    }
}
