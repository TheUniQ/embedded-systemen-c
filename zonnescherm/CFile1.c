/*
	Na restart of bij het opstarten haalt hij alle waardes van de variable hierboven uit de eeprom 
*/
void get_eeprom(){
	max_dist = eeprom_read_byte((uint8_t *)0); //<-- moet in eeprom
	min_dist = eeprom_read_byte((uint8_t *)1); //<-- moet in eeprom
	toggle_temp = eeprom_read_byte((uint8_t *)2); //<-- moet in eeprom
	toggle_light = eeprom_read_byte((uint8_t *)3);  //<-- moet in eeprom
	toggle_type =  eeprom_read_byte((uint8_t *)4); //0= manual, 1=temp, 2=light, 3= temp || light, 4=temp && light //<-- moet in eeprom
	roll_status = eeprom_read_byte((uint8_t *)101); // de status van de roll~`
	PORTB = (roll_status * 2) + 2; // als roll status 0 is dan 2/ roll status 1 dan 4 | 0x02 is in 0x04 is out
}

/*
	Read de ADC van de analoge pins, ch = de pin, 0= licht, 1=temp
*/
uint16_t read(uint8_t ch){
	ADMUX=(1<<REFS0); // reset admux
	ch &= 0x07;
	ADMUX |= ch;
	ADCSRA|=(1<<ADSC);
	loop_until_bit_is_clear(ADCSRA, ADSC);
	return(ADC);
}

/*
	Kijkt of de licht sensoor of de temperatuur sensor is aangesloten. de analoge input draden zijn verbonden met een 10k resistor dus als er niks op aangesloten is is de waarde altijd 0
	Licht kan nooit 0 zijn want het is 1 - 102
	temperatuur kan nooit 0 zijn of het moet -115 graden zijn <-- denk niet dat het vaak voorkomt/ of 141 graden <-- maar denk ook niet dat dat vaak voor komt.
	als hij een 0 terug geeft is hij niet aangesloten
*/

/*
void is_connected()
{
	if(read(0) == 0)
	{
		licht_is_connected = 0;
	}
	else
	{
		licht_is_connected = 1;
	}
	if(read(1) == 0)
	{
		temp_is_connected = 0;
	}
	else 
	{
		temp_is_connected = 1;
	}
}
*/
void is_connected(){
	licht_is_connected = ((read(0) == 0) ? 0 :1);
	temp_is_connected = ((read(1) == 0) ? 0 :1);
}

/*
	Transmit byte 
*/
void transmit(uint8_t data)
{
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 =	data;
}

/*
	bij command 225, krijg je deze informatie.
*/
void send_data(){
	is_connected();
	if(licht_is_connected == 1){transmit(average_licht);}
	if(temp_is_connected == 1){transmit(average_temp);}
	transmit(dist);
}

/*
	verstuur de informatie van de com device, en of de licht sensor/temperatuur sensor is aangeloten.
	command = 227
*/
void send_info(){
	is_connected();
	char what[] = {'C', 'o', 'm', '_', 'R', 'e', 'n', 'e', '|', temp_is_connected, '|', licht_is_connected};
	for (int i = 0; i < sizeof(what); i++)	{
		transmit(what[i]);
	}
}

/*
	berekend de gemiddelde data van licht en temp van store data
	en zal deze elke 40 seconden berekenen.
*/
void calc_average_data(){
	uint16_t sum_light = 0;
	float sum_temp = 0;
	for(uint8_t i=0; i<40; i++){
		sum_light = sum_light + licht_list[i];
		sum_temp = sum_temp + temp_list[i];
	}
	average_licht = (sum_light/400); // (sum_light / 40) /10 == sum_light / 400
	average_temp = (sum_temp/40);
}

/*
	sla de temperatuur en licht elke seconden op. en na 40 seconden roept hij calc_average_data aan
*/
void store_data(){
	if (list_counter < 40){
		licht_list[list_counter] = read(0);
		temp = read(1);
		//transmit(temp);
		temp = temp * (5000.0 / 1024);
		temp = ((temp -750) /10) +25;
		temp_list[list_counter] = temp;		
		list_counter++;	
	}else{
		list_counter = 0;
		calc_average_data();
	}
}

/*
	get_distance =  elke 10 ms stuur hij een 10us pulse door de trigger van de ultrasonic sensor. interupts doen het verder.
*/
void get_distance(){
	PORTD &=~ (1 << PIND4);
	_delay_us(1);
	PORTD |= (1 << PIND4);
	_delay_us(10);
	PORTD &=~ (1 << PIND4);
	working = 1;
}

/*
	zit een 1 minuut delay op vanwage de gemiddelde temperatuur, die is altijd 0 in de eerste 40 seconden , en deze in de eeprom oplaan is niet handig, dan is de eeprom na 40 dagen stuk.
	als toggle type 0 is, dan doet hij niks
	als toggle type 1 is, dan gaat hij alleen op temperatuur.
	als toggle type 2 is, dan gaat hij alleen op licht.
	als toggle type 3 is, dan gaat hij op temperatuur of licht.
	als toggle type 4 is, dan gaat hij op temperatuur en licht.
*/


void auto_modus()
{
	if(toggle_type != 0)
	{
		if(toggle_type == 1)
		{
			if(roll_status == 0 && average_temp > toggle_temp)
			{
				rollout();
			}
			else if(roll_status ==1 && average_temp < toggle_temp)
			{
				rollin();
			}
		}
		else if(toggle_type == 2)
		{
			if(roll_status == 0 && average_licht > toggle_light)
			{
				rollout();
			}
			else if(roll_status ==1 && average_licht < toggle_light)
			{
				rollin();
			}
		}
		else if(toggle_type ==3)
		{
			if(roll_status == 0)
			{
				if(average_licht > toggle_light || average_temp > toggle_temp)
				{
					rollout();
				}
			}
			else if(roll_status ==1)
			{
				if(average_licht < toggle_light && average_temp < toggle_temp)
				{
					rollin();
				}
			}
		}
		else if(toggle_type ==4)
		{
			if(roll_status == 0)
			{
				if(average_licht > toggle_light && average_temp > toggle_temp)
				{
					rollout();
				}
			}
			else if(roll_status ==1)
			{
				if(average_licht < toggle_light || average_temp < toggle_temp)
				{
					rollin();
				}
			}
		}
	}	
}

/*
void auto_modus(){
	switch(toggle_type){
		case 0:
		break;
		
		case 1:
		if(roll_status == 0 && average_licht > toggle_light){rollout();}
		if(roll_status == 1 && average_licht < toggle_light){rollin();}
		break;

		case 2:
		if(roll_status == 0 && average_temp > toggle_temp){rollout();}
		if(roll_status == 1 && average_temp < toggle_temp){rollin();}
		break;

		case 3:
		if (roll_status == 0 && (average_licht > toggle_light || average_temp > toggle_temp)){rollout();}
		if (roll_status == 1 && (average_licht < toggle_light && average_temp < toggle_temp)){rollin();}
		break;
					
		case 4:
		if (roll_status == 0 && average_licht > toggle_light && average_temp > toggle_temp){rollout();}
		if (roll_status == 1 && average_licht < toggle_light || average_temp < toggle_temp){rollin();}
		break;
	}
}
*/
/*
	hier rollt hij. roll taak word toegevoegd bij de rollout/ rollin funcite, en verwijderd als deze klaar is.
*/
void roll(){
	if (roll_out == 1){
		PORTB ^= 0x01;
		if (dist >= max_dist){
			roll_out = 0;
			PORTB =0x04;
			eeprom_update_byte(101, 1);
			roll_status = 1; //als hij helemaal is uitgerold
			uint8_t task = sizeof(SCH_tasks_G) -1;
			SCH_Delete_Task(task);
		}
	}
	if (roll_in == 1){
		PORTB ^= 0x01;
		if (dist <= min_dist){
			roll_in = 0;
			eeprom_update_byte(101, 0);
			roll_status = 0;
			PORTB =0x02;
			uint8_t task = sizeof(SCH_tasks_G) -1;
			SCH_Delete_Task(task);			
		}
	}
}

/*
	hier rollt hij uit en maakt hij de roll taak aan.
*/
void rollout(){
	if (roll_out == 0){
		PORTB = 0x04;
		roll_in = 0;
		roll_out = 1;
		SCH_Add_Task(roll, 0, 50);
	}
}

/*
	hier rollt hij in en maakt hij de roll taak aan.
*/
void rollin(){
	if (roll_in == 0){
		PORTB = 0x02;
		roll_out =0;
		roll_in = 1;
		SCH_Add_Task(roll, 0, 50);
	}
}

void get_settings(){
	transmit(max_dist);
	transmit(min_dist);
	transmit(toggle_temp);
	transmit(toggle_light);
	transmit(toggle_type);	
}

void reset_settings(){
	eeprom_update_byte(100, 0); //zet de byte op 100 op 0 zodat hij de eeprom init kan uitvoeren.
	eeprom_init(); // zet alle standaard instellingen.
	get_eeprom(); // zet alle settings op de standaard instellingen.
}

/*
	hier kommen alle bytes die binnen komen binnen.
	225 = send data = informatie over de gemiddelde temperatuur en licht.
	226 = informatie over alle ingestelde instellingen.
	227 = send info = de naam van de com device en welke sensoren er zijn aangesloten
	228 = standaard instellingen.
	1 = handmatig naar beneden, hij zal ook de handmatige setting aanzetten
	2 = handmatig naar boven, hij zal ook de handmatige setting aanzetten
	200= setting handmatig instellen.
	200 syntax = 200 -voor command, <0-4> - de instelling die je wilt veranderen, <waarde> de waarde die je wilt instellen.
	200 syntax = 200 0 100 = zet max dist op 100 cm | 0 -255
	200 syntax = 200 1 005 = zet min dist op 5 cm | 0 -255
	200 syntax - 200 2 024 = zet de temperatuur waar de scherm schakelt.| -128 - 127
	200 syntax= 200 3 090 = zet de licht waar de scherm schakelt.| 0 -100
	200 syntax= 200 4 003 = zet de waarde voor de toggle |  0-4 | 0 is handmatig, 1 = temperatuur, 2= licht, 3= licht of temperatuur, 4= licht en temperatuur.
*/
void commando(){
	if bit_is_set(UCSR0A,RXC0) {
		uint8_t Receivedbyte = UDR0;
		switch(Receivedbyte){
			case 225:
			send_data();
			break;
			
			case 226:
			get_settings();
			break;
			
			case 227:
			send_info();
			break;
			
			case 228:
			reset_settings();
			break;

			case 1:
			toggle_type = 0; //toggle type naar handmatig niet in eeprom zetten, dan zou hij altijd in handmatige stand blijven staan, nu is hij na een restart weer naar de eeprom waarde
			rollout();
			break;
			
			case 2:
			toggle_type = 0; //toggle type naar handmatig 
			rollin();
			break;
			
			case 200:
			loop_until_bit_is_set(UCSR0A,RXC0);
			unsigned char setting = UDR0; //zie command regel boven functie.
			_delay_us(1);
			loop_until_bit_is_set(UCSR0A,RXC0);
			uint8_t value = UDR0;
			set_setting(setting, value);
			break;
			
			case 201:
			is_connected();
			break;
		}
	}
}

/*
void commando()
{
	if bit_is_set(UCSR0A,RXC0)
	{
		uint8_t Receivedbyte = UDR0;
		if (Receivedbyte == 225)
		{
			send_data();
		}
		else if (Receivedbyte == 226)
		{
			transmit(max_dist);
			transmit(min_dist);
			transmit(toggle_temp);
			transmit(toggle_light);
			transmit(toggle_type);	
		}
		else if (Receivedbyte == 227)
		{
			send_info();
		}
		else if (Receivedbyte == 228)
		{
			eeprom_update_byte(100, 0); //zet de byte op 100 op 0 zodat hij de eeprom init kan uitvoeren.
			eeprom_init(); // zet alle standaard instellingen.
			get_eeprom(); // zet alle settings op de standaard instellingen.
		}
		else if (Receivedbyte == 1)
		{
			toggle_type = 0; //toggle type naar handmatig niet in eeprom zetten, dan zou hij altijd in handmatige stand blijven staan, nu is hij na een restart weer naar de eeprom waarde
			rollout();
		}
		else if(Receivedbyte == 2)
		{
			toggle_type = 0; //toggle type naar handmatig 
			rollin();
		}
		else if(Receivedbyte == 200)
		{
			loop_until_bit_is_set(UCSR0A,RXC0);
			unsigned char setting = UDR0; //zie command regel boven functie.
			_delay_us(1);
			loop_until_bit_is_set(UCSR0A,RXC0);
			uint8_t value = UDR0;
			set_setting(setting, value);
		}
		else if(Receivedbyte == 201)
		{
			is_connected();
		}
	}
	
}
*/


/*
	deze functie zet de waardes voor de instellingen, en slaat deze op in de eeprom.
	Settings:
	0 = max_dist = byte
	1 = min_dist = byte
	2 = toggle_temp = int
	3 = toggle_light = byte
	4 = toggle_type = unsigned char
*/
/*
void set_setting(unsigned char setting, uint8_t value)
{
	
	if(setting == 0)
	{
		max_dist = value;
		eeprom_update_byte(0, max_dist); // 0 -255
	}
	else if(setting ==1)
	{
		min_dist = value;
		eeprom_update_byte(1, min_dist); //5 255 <-- onder de 2~5 slaat die afstandmeter op hol.
	}
	else if(setting ==2)
	{
		toggle_temp = value;
		eeprom_update_byte(2, toggle_temp); // -128 - 127 ~ 
	}
	else if(setting ==3)
	{
		toggle_light = value;
		eeprom_update_byte(3, toggle_light);// 0 - 102 te sensor zal nooit 0 aangeven behalve als deze niet is aangesloten.
	}
	else if(setting ==4)
	{
		toggle_type = value;
		eeprom_update_byte(4, toggle_type);//0 = handmatig, 1 is temperatuur, 2 is licht, 3 is licht of temperatuur, 4 is licht en temperatuur. <-- 0-4
	}
}
*/
void set_setting(unsigned char setting, uint8_t value){
	switch(setting){
		case 0:
		max_dist = value;
		eeprom_update_byte(0, max_dist); // 0 -255
		break;
		
		case 1:
		min_dist = value;
		eeprom_update_byte(1, min_dist); //5 255 <-- onder de 2~5 slaat die afstandmeter op hol.
		break;
		
		case 2:
		toggle_temp = value;
		eeprom_update_byte(2, toggle_temp); // -128 - 127 ~ 
		break;
		
		case 3:
		toggle_light = value;
		eeprom_update_byte(3, toggle_light);// 0 - 102 te sensor zal nooit 0 aangeven behalve als deze niet is aangesloten.
		break;
		
		case 4:
		toggle_type = value;
		eeprom_update_byte(4, toggle_type);//0 = handmatig, 1 is temperatuur, 2 is licht, 3 is licht of temperatuur, 4 is licht en temperatuur. <-- 0-4
		break;
	}
}

/*
	Interupt voor int1/ als de echo van de ultrasonic sensor een signaal krijgt word deze interupt aangeroepen. 
	de eerste keer met de rising edge. dan reset hij de timer en count. 
	de tweede keer falling edge.. telt hij de count en de timer bij elkaar op en berekent de afstand
	afstand is niet constant. als de temperatuur met 5 graden verschilt, veranderd de afstand al een paar cm per meter.
*/
ISR (INT1_vect){
	if(working == 1){
		if(rising_edge == 0){
			rising_edge = 1;
			TCNT0 = 0;
			count = 0;
		} else {
			rising_edge = 0;
			dist = ((count * 256) + TCNT0) / 933 /** 933 =  1 / 0.001071875 = 1 / 16000 * 34.3 /2 */;
			//uint8_t tempu = dist;
			//transmit(tempu);
			//uint8_t * p;
			// p =(uint8_t *)&dist;
			//transmit(*p++);
			//transmit(*p++);
			//transmit(*p++);
			//transmit(*p);
			working = 0;
		}
	}
	//0,0010625= magie
}

/*
	overflow timer/ als de timer0 opverlowd krijgt count een extra waarde count is een 32 bit getal.
	dus 1- 4miljard..
	8 bit = 2^8 * 256 / 933 = zou 70 cm moeten zijn maar 2^16 * 256 /933 is blijkbaar 70 cm... daarom 32 bit getal.
*/
ISR(TIMER0_OVF_vect){
	if(rising_edge == 1){
		count++;
	}
}



