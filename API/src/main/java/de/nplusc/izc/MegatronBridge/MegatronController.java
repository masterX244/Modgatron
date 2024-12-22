package de.nplusc.izc.MegatronBridge;

import de.nplusc.izc.MegatronBridge.Models.CustomAudio;
import de.nplusc.izc.MegatronBridge.Models.MovementActions;
import de.nplusc.izc.MegatronBridge.Models.OnboardAudio;
import de.nplusc.izc.MegatronBridge.Models.RawAction;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.media.Content;
import io.swagger.v3.oas.annotations.responses.ApiResponse;
import io.swagger.v3.oas.annotations.responses.ApiResponses;
import io.swagger.v3.oas.annotations.tags.Tag;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.multipart.MultipartFile;

@RestController
@Tag(name = "base", description = "API module for the Megatron sculpture at the CC86 assembly at 38C3")
public class MegatronController
{
    @Autowired
    private MegatronService svc;


    @Operation(summary = "Sets the volume level, range is from 0 to 8", description = "Sets the volume for the custom audio to the given level")
    @ApiResponses(value = {
            @ApiResponse(responseCode = "200", description = "successful operation"),
            @ApiResponse(responseCode = "400", description = "Out of range value",content = @Content)}
    )
    @PostMapping("/volume")
    @ResponseBody
    public void setVolume(int volume)
    {
        svc.setVolume(volume);
    }

    @Operation(summary = "Raw lowlevel command, debugging only", description = "Raw command sending, don't waste your time if you don't have the secret key")
    @ApiResponses(value = {
            @ApiResponse(responseCode = "200", description = "successful operation"),
            @ApiResponse(responseCode = "401", description = "Not a chance!", content = @Content)}
    )
    @PostMapping("/debug")
    @ResponseBody
    public void executeRaw(RawAction action)
    {
        svc.executeRaw(action);
    }

    @Operation(summary = "Triggers one of the onboard audio snippets", description = "Plays one of the original builtin audio snippets, WARNING: LOUD!")
    @ApiResponses(value = {
            @ApiResponse(responseCode = "200", description = "successful operation"),
            @ApiResponse(responseCode = "400", description = "Out of range value", content = @Content)}
    )
    @PostMapping("/onboardAudio")
    @ResponseBody
    public void playOnbardAudio(OnboardAudio audio)
    {
        svc.playOnbardAudio(audio);
    }

    @Operation(summary = "Triggers one of the custom fixed audio snippets", description = "Plays one of the fixed custom audio snippets. Volume depends on configured volume")
    @ApiResponses(value = {
            @ApiResponse(responseCode = "200", description = "successful operation"),
            @ApiResponse(responseCode = "400", description = "Out of range value", content = @Content)}
    )
    @PostMapping("/customAudio")
    @ResponseBody
    public void playCustomAudio(CustomAudio audio)
    {
        svc.playCustomAudio(audio);
    }
    @Operation(summary = "Lists the valid custom audio snippets", description = "Lists the valid custom audio snippets")
    @ApiResponses(value = {
            @ApiResponse(responseCode = "200", description = "successful operation")
    })
    @GetMapping("/customAudio")
    public String[] listAudio()
    {
        return svc.listAudio();
    }

    @Operation(summary = "Triggers the fixed movement sequences", description = "Triggers one of the fixed movement actions")
    @ApiResponses(value = {
            @ApiResponse(responseCode = "200", description = "successful operation"),
            @ApiResponse(responseCode = "400", description = "Out of range value", content = @Content)}
    )
    @PostMapping("/wiggle")

    public void wiggle(MovementActions action)
    {
        svc.wiggle(action);
    }

    @Operation(summary = "toggles mouth wiggling state", description = "Plays one of the original builtin audio snippets, WARNING: LOUD!")
    @ApiResponses(value = {
            @ApiResponse(responseCode = "200", description = "successful operation"),
            @ApiResponse(responseCode = "400", description = "Out of range value", content = @Content)}
    )
    @PostMapping("/wiggleMouth")
    public void wiggleMouth()
    {
        svc.wiggleMouth();
    }



    @Operation(summary = "send raw audio to be played", description = "plays raw 11khz 8bit signed PCM audio (max 60seconds)")
    @PostMapping(value = "dynamicAudio",
            consumes = MediaType.MULTIPART_FORM_DATA_VALUE)
    public void streamAudio(@RequestParam("file") MultipartFile file)
    {
        svc.streamAudio(file);
    }
    //TODO sent audio
}
