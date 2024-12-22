package de.nplusc.izc.MegatronBridge;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.web.servlet.config.annotation.EnableWebMvc;


@SpringBootApplication
@EnableWebMvc
public class MegatronBridgeApplication {

	public static String PSK = "";
	public static SerialSpammer spammer = null;
	public static void main(String[] args)
	{
		if(args.length<2)
		{
			System.err.println("missing arguments, serial port and preshared key are both required");
		}
		spammer = new SerialSpammer(args[0]);
		PSK=args[1];

		SpringApplication.run(MegatronBridgeApplication.class, args);
	}

}
