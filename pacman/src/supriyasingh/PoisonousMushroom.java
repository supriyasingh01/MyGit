/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package supriyasingh;

/**
 *
 * @author supriyasingh
 */
public class PoisonousMushroom extends Entity {

    @Override
    public void create() {
        for (int i = 0; i < 5; i++) {
            while (true) {
                int randX = 0 + (int) (Math.random() * 19);
                int randY = 0 + (int) (Math.random() * 49);
                //System.out.println("rand x and rany =" + randX+randY);
                if ((Maze.available(randX, randX)) && (Maze.MAZE[randX][randY] != 'A') && (Maze.MAZE[randX][randY] != 'P') && (Maze.MAZE[randX][randY] != '*') && (Maze.MAZE[randX][randY] != '@')) {
                    if (this.getClass().equals(PoisonousMushroom.class)) {
                        Maze.MAZE[randX][randY] = 'M';
                        break;
                    } else {
                        System.out.println("no place for poisonous mushroom");
                    }
                }
            }
        }
    }
}
