import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JButton;

public class BasicSwingSimple extends JFrame{
    
    JPanel p = new JPanel();
    JButton b = new JButton("Hello");
    
    public static void main(String args[]){
        new BasicSwingSimple();
    }
    
    public BasicSwingSimple(){
        super("Basic Swing App");
        setSize(400,300);
        p.add(b);
        add(p);
        setVisible(true);
    }
    
}
