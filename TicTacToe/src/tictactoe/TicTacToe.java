/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package tictactoe;

import java.awt.GridLayout;
import javax.swing.JFrame;
import javax.swing.JPanel;

/**
 *
 * @author supriyasingh
 */
public class TicTacToe extends JFrame{

    JPanel p = new JPanel();
    XOButton[] buttons = new XOButton[9];
    public static void main(String[] args) {
        // TODO code application logic here
        new TicTacToe();
    }
    
    public TicTacToe(){
        super("TICTACTOE");
        setSize(800,800);
        setResizable(false);
        setDefaultCloseOperation(EXIT_ON_CLOSE);//This makes sure the program closes when the window is closed
        p.setLayout(new GridLayout(3,3));
        
        for(int i = 0; i <9 ;i++){
            buttons[i] = new XOButton();
            p.add(buttons[i]);
        }
        add(p);
        setVisible(true);
    }
}
