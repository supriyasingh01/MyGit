/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package supriyasingh;

/**
 *
 * @author supriyasingh
 */
public interface EntityInterface {
    
    abstract void create();
    abstract char move(int x1, int y1, int x2, int y2, char s);
    abstract char getSymbol();
}
