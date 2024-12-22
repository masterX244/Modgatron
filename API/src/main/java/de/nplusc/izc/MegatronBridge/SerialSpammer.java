package de.nplusc.izc.MegatronBridge;


import com.fazecast.jSerialComm.SerialPort;

import java.io.*;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.Queue;

enum NextChunkAction
{
    WAIT,
    REPEAT,
    CONTINUE,
    FINISHED
}

public class SerialSpammer
{
    private InputStream in = null;
    private OutputStream o = null;
    private NextChunkAction nextChunkReady = NextChunkAction.WAIT;
    private int waitCtr = 0;
    private Object workaround = new Object();
    private int streampointer = 0;
    private Queue<CommandQueueItem> queue= new LinkedList<>();
    private boolean startedStreamPlayback = false;

    public Queue<CommandQueueItem> getQueue()
    {
        return queue;
    }

    private final char[] hex = "0123456789ABCDEF".toCharArray();
    private byte[] streamfile = null;
    public SerialSpammer(String comPortName)
    {

        System.out.println("Gobblygob");

        int sendOffset = 0;
        SerialPort comPort = SerialPort.getCommPort(comPortName);
        comPort.openPort();
        comPort.setComPortTimeouts(SerialPort.TIMEOUT_READ_SEMI_BLOCKING, 0, 0);
        in = comPort.getInputStream();
        o = comPort.getOutputStream();
        new Thread(()->

        {
            System.out.println("MunchMunchMunch");
            try
            {
                while (true)
                {
                    if(in.available()<1)
                    {
                        continue;
                    }
                    int read = in.read();
                    if (read == '~')
                    {
                        System.out.println("RSME");
                        nextChunkReady = NextChunkAction.CONTINUE;
                        synchronized (workaround)
                        {
                            System.out.println("POKE");
                            workaround.notify();
                        }

                    }
                    if (read == '<')
                    {
                        nextChunkReady = NextChunkAction.REPEAT;
                        synchronized (workaround)
                        {
                            System.out.println("POKE");
                            workaround.notify();
                        }
                    }

                    System.out.write(read);
                    System.out.flush();
                }
            }
            catch (IOException e)
            {
                e.printStackTrace();
            }
        }).start();

        new Thread(()->

        {
        while(true)
        {
            CommandQueueItem itm = queue.poll();
            if(itm != null)
            {
                switch (itm.type)
                {
                    case ServoAction:
                        if(itm.argument>15||itm.argument<0)
                        {
                            break;
                        }
                        try
                        {
                            o.write('*');
                            o.write('S');
                            o.write(hex[itm.argument]);
                            o.write('G');
                        } catch (IOException e)
                        {
                            e.printStackTrace();
                        }
                        break;
                    case OnboardAudio:
                        if(itm.argument>15||itm.argument<0)
                        {
                            break;
                        }
                        try
                        {
                            o.write('*');
                            o.write('T');
                            o.write(hex[itm.argument]);
                            o.write('G');
                        } catch (IOException e)
                        {
                            e.printStackTrace();
                        }
                        break;
                    case CustomAudio:
                        if(itm.argument<0)
                        {
                            break;
                        }
                        boolean wiggleMouth = false;
                        if(itm.argument>0xFFFF)
                        {
                            wiggleMouth=true;
                            itm.argument&=0xFFFF;
                        }
                        System.out.println(itm.argument);
                        LinkedList<Character> reverser = new LinkedList<>();
                        int arg = itm.argument;
                        while(arg>0)
                        {
                            reverser.add(hex[arg%16]);
                            arg>>=4;
                        }
                        try
                        {
                            o.write('*');
                            while(true)
                            {
                                Character c = reverser.pollLast();
                                if(c==null)
                                {
                                    break;
                                }
                                o.write(c);
                                System.out.println("CSEND="+c);
                            }
                            o.write(wiggleMouth?'V':'v');
                        } catch (IOException e)
                        {
                            e.printStackTrace();
                        }

                        break;

                    case StreamAudio:
                        if(streamfile==null)
                        {
                            streamfile = itm.streamData;
                            nextChunkReady=NextChunkAction.CONTINUE;
                            synchronized (workaround)
                            {
                                System.out.println("POKE");
                                workaround.notify();
                            }
                        }
                        break;
                    case MundwerkToggle:
                        try
                        {
                            o.write('M');
                            try
                            {
                                Thread.sleep(10);
                            } catch (InterruptedException e)
                            {
                                e.printStackTrace();
                            }
                        } catch (IOException e)
                        {
                            e.printStackTrace();
                        }
                        break;
                    case RawCommand:
                        try
                        {
                            for(char c:itm.extendedArg.toCharArray())
                            {
                                o.write(c);
                            }
                        } catch (IOException e)
                        {
                            e.printStackTrace();
                        }
                        break;
                    case VolumeSetter:
                        try
                        {
                            for(int i=0;i<8;i++)
                            {
                                    o.write('-');
                            }
                            for(int i=0;i<itm.argument;i++)
                            {
                                o.write('+');
                            }
                        } catch (IOException e)
                        {
                            e.printStackTrace();
                        }
                    break;
                }
            }
            if(streamfile!=null)
            {
                try{
                if(streampointer>=streamfile.length)
                {
                    System.out.println("FINI");
                    System.err.println("FINI");
                    streamfile=null;
                    streampointer=0;
                    startedStreamPlayback=false;
                }
                else
                {
                    if((!startedStreamPlayback)&&(streampointer==8192||streampointer==streamfile.length))
                    {
                        System.out.println("PLAY");
                        o.write('|');
                        startedStreamPlayback=true;
                    }
                    switch(nextChunkReady)
                    {
                        case REPEAT:
                            System.out.println("RTRY=("+streampointer+")");
                            waitCtr=0;
                            if(streampointer>=64)
                            {
                                streampointer-=64;
                            }
                        case CONTINUE:
                            waitCtr=0;
                            System.out.println("STR=("+streampointer+")");
                            byte[] readbfr = Arrays.copyOfRange(streamfile,streampointer,streampointer+64);
                            streampointer+=64;
                            o.write('>');
                            nextChunkReady=NextChunkAction.WAIT;
                            o.write(readbfr);

                            break;
                    }
                    synchronized (workaround)
                    {
                        if(nextChunkReady==NextChunkAction.WAIT)
                        {
                            System.out.println("GOTO WAIT");
                            try
                            {
                                workaround.wait();
                            } catch (InterruptedException e)
                            {
                                e.printStackTrace();
                            }
                        }
                    }
                }
                }
                catch (IOException e)
                {
                    System.out.println("Zöinks");
                    e.printStackTrace();
                }
            }
        }
        }).start();
    }
}
