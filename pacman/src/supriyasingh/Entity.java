/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package supriyasingh;

/**
 *
 * @author supriyasingh
 */
abstract public class Entity implements EntityInterface{
    
    char Symbol;
    int XPosition;// X pos of entity in maze
    int YPosition;// X pos of entity in maze
    
    //This method is implemented in Squirrel class
    public abstract void create();{}
    
    //Moves an entity from location(x1,y1) to location(x2,y2). Returns a character that was replaced in the new position
    //Enttity is idenified by 's'
    @Override
    public char move(int x1, int y1, int x2, int y2, char s){
        return '@';
    }
    
    @Override
    public char getSymbol(){
        return this.Symbol;
    }
    
    void setX(int XPosition){
        this.XPosition = XPosition;
    }
    
    int getX(){
        return this.XPosition;
    }
    
    void setY(int YPosition){
        this.YPosition = YPosition;
    }
    
    int getY(){
        return this.YPosition;
    }
}
