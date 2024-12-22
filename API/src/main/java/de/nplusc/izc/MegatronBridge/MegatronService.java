package de.nplusc.izc.MegatronBridge;

import de.nplusc.izc.MegatronBridge.Models.CustomAudio;
import de.nplusc.izc.MegatronBridge.Models.MovementActions;
import de.nplusc.izc.MegatronBridge.Models.OnboardAudio;
import de.nplusc.izc.MegatronBridge.Models.RawAction;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Service;
import org.springframework.util.ResourceUtils;
import org.springframework.web.multipart.MultipartFile;
import org.springframework.web.server.ResponseStatusException;

import java.io.*;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;

@Service
public class MegatronService
{
    public void setVolume(int volume)
    {

        if(volume>0&&volume<=8)
        {
            CommandQueueItem itm = new CommandQueueItem();
            itm.argument=volume;
            itm.type=CommandType.VolumeSetter;
            MegatronBridgeApplication.spammer.getQueue().add(itm);
        }
        else
        {
            throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "Out of Range");
        }
    }
    public void executeRaw(RawAction action)
    {
        boolean sus=false;
        if(action.getpsk()==null)
        {
            sus=true;
            System.out.println("it smells fishy");
        }
        if(action.getCommand()==null)
        {
            sus=true;
            System.out.println("it smells fishy again");
        }

        if(!sus&&action.getpsk().equals(MegatronBridgeApplication.PSK))
        {
            CommandQueueItem itm = new CommandQueueItem();
            itm.type=CommandType.RawCommand;
            itm.extendedArg =action.getCommand();
            MegatronBridgeApplication.spammer.getQueue().add(itm);
        }
        else
        {
            throw new ResponseStatusException(HttpStatus.UNAUTHORIZED, "from __future__ import braces");
        }
    }
    public void playOnbardAudio(OnboardAudio audio)
    {
        int internalId = switch(audio)
        {
            case DECEPTICONSRETREAT -> 0; //TODO checken dass die stimmen und vervollständigen
            case ALLHAILMEGATRON -> 1;
            case BZZZZZZZRT -> 2;
            case DECEPTICONSATTACK ->3;
            case SND_EYELIGHT_ON -> 4;
            case SND_EYELIGHT_OFF -> 5;
            case SND_BOOTUP -> 6;
        };
        CommandQueueItem itm = new CommandQueueItem();
        itm.type=CommandType.OnboardAudio;
        itm.argument=internalId;
        MegatronBridgeApplication.spammer.getQueue().add(itm);
    }

    public String[] listAudio()
    {
        return audioMapTable.stream().map((o)->o.ID()).toList().toArray(new String[audioMapTable.size()]);
    }

    public void playCustomAudio(CustomAudio audio)
    {
        final String ref = audio.getTargetSample();
        InternalAudioMapping mapping = audioMapTable.stream().filter(o->o.ID().equals(ref)).findFirst().orElseGet(()->null);
        if(mapping==null)
        {
            throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "from __future__ import braces");
        }
        int internalId=mapping.index();
        if(audio.isWiggleMouth())
        {
            internalId|=0x1_00_00;
        }
        CommandQueueItem itm = new CommandQueueItem();
        itm.type=CommandType.CustomAudio;
        itm.argument=internalId;
        MegatronBridgeApplication.spammer.getQueue().add(itm);
    }

    public void wiggle(MovementActions action)
    {
        int internalId = switch(action)
        {
            case EYEGLOW -> 0x6;
            case EYEDARK -> 0xA;

            case EYEFLICKERING -> 0x2;
            case EARWIGGLE -> 0xC;
            case BATTLEMODEON -> 0xE;
            case BATTLEMODEOFF -> 0x1;
        };
        CommandQueueItem itm = new CommandQueueItem();
        itm.type=CommandType.ServoAction;
        itm.argument=internalId;
        MegatronBridgeApplication.spammer.getQueue().add(itm);
    }
    public void wiggleMouth()
    {
        System.out.println("Mundwerk wiggler");
        CommandQueueItem itm = new CommandQueueItem();
        itm.type=CommandType.MundwerkToggle;
        MegatronBridgeApplication.spammer.getQueue().add(itm);
    }

    public void streamAudio(MultipartFile file)
    {
        if(file.getSize()>60*11000)
        {
            throw new ResponseStatusException(HttpStatus.PAYLOAD_TOO_LARGE, "forget it....");
        }
        try
        {
            CommandQueueItem itm = new CommandQueueItem();
            itm.type=CommandType.StreamAudio;
            byte[] tmp = file.getBytes();
            itm.streamData= Arrays.copyOfRange(tmp,0,tmp.length&(~0b00111111)); //trim down to nearest 64 byte chunk
            MegatronBridgeApplication.spammer.getQueue().add(itm);
        } catch (IOException e)
        {
            e.printStackTrace();
            throw new ResponseStatusException(HttpStatus.INTERNAL_SERVER_ERROR, "oupps");
        }
    }

    private List<InternalAudioMapping> audioMapTable = loadMappings();

    private static List<InternalAudioMapping> loadMappings(){
        LinkedList list = new LinkedList<>();
        try (BufferedReader in = new BufferedReader(new FileReader(ResourceUtils.getFile("classpath:audiomapping.lst")));){

            in.lines().forEach(line->
            {
                String[] linesplit = line.split(":");
                int index = Integer.decode(linesplit[1]);
                list.add(new InternalAudioMapping(linesplit[0],index));
            });

        } catch (IOException e) {
            e.printStackTrace();
        }
        return list;
    }
}



record InternalAudioMapping(String ID, int index)
{}
