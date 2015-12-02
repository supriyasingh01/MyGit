/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package supriyasingh;

/**
 *
 * @author supriyasingh
 */
public class Nuts extends Entity {

    static final int TotalNuts = 5;
    Nuts[] nut = new Nuts[TotalNuts];
    int almondCount = 0, peanutCount = 0;

   //Randomly creates the location where nuts are placed
    @Override
    public void create() {
 
        for (int i = 0; i < TotalNuts; i++) {
            int randomNut = 1 + (int) (Math.random() * 2); // Randomly chossing type of nut
            if (randomNut == 1) {
                nut[almondCount] = new Almond();
                nut[almondCount].placingNunts();
                almondCount++;
            } else {
                nut[peanutCount] = new Peanut();
                nut[peanutCount].placingNunts();
                peanutCount++;
            }
        }
    }

    //Placing nuts in maze
    public void placingNunts() {
        while (true) {
            int randX = 0 + (int) (Math.random() * 19);
            int randY = 0 + (int) (Math.random() * 49);
            //System.out.println("rand x and rany =" + randX+randY);
            if ((Maze.available(randX, randX)) && (Maze.MAZE[randX][randY] != 'A') && (Maze.MAZE[randX][randY] != 'P') && (Maze.MAZE[randX][randY] != '*') && (Maze.MAZE[randX][randY] != '@')) {
                if (this.getClass().equals(Almond.class)) {
                    Maze.MAZE[randX][randY] = 'A';
                    return;
                } else if (this.getClass().equals(Peanut.class)) {
                    Maze.MAZE[randX][randY] = 'P';
                    return;
                } else {
                    System.out.println("The nut is neither Almond nor Peanut");
                }
            }
        }
    }

//    @Override
//    public char move(int x1, int y1, int x2, int y2, char s) {
//        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
//    }

    @Override
    public char getSymbol() {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    static int isNuts(char symbol) {
        if (symbol == 'A') {
            return 5;//Nutrition pints for almonds
        } else if (symbol == 'P') {
            return 10;//Nutrition pints for peanuts
        } else {
            return 0;
        }
    }
}
