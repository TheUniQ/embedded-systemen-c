sTask SCH_tasks_G[SCH_MAX_TASKS];

uint16_t licht_list[40];
float temp_list[40];
uint8_t average_licht = 0;
uint8_t average_temp = 0;
uint8_t list_counter = 0;
float temp;
unsigned char licht_is_connected;
unsigned char temp_is_connected;

uint32_t count = 0; // kan de timer0 2^32 keer tellen. 2^8 = 70 cm 2^16 = 17000 cm
unsigned char rising_edge =0;
unsigned char working = 0;
float dist;

unsigned char roll_out;
unsigned char roll_in; 
unsigned char roll_status;

uint8_t max_dist;
uint8_t min_dist;
int toggle_temp;
uint8_t toggle_light;
unsigned char toggle_type;

void init(){
	init_ports();
	init_int1();
	init_timer0();
	uart_init();
	ADC_init();
	eeprom_init();
}

void init_ports(){
	DDRB = 0xff;
	DDRD |= (1<<PORTD4); // Portd 4 op output, vanwege de ultrasonic sensor
	DDRD &=~ (1<<PORTD3); // portd 3 op input, vanwege de echo
}

void init_int1(){
	EICRA = (1 << ISC10); //enable trigger met falling edge en rising edge
	EIMSK = (1 << INT1);// enable int1 
}

void init_timer0(){
	TCCR0B = _BV(CS10);//
	TCNT0 =0;
	TIMSK0 = (1<<TOIE0);
}

void uart_init(){
	UBRR0H = 0;
	UBRR0L = UBBRVAL;
	UCSR0A = 0;
	UCSR0B = _BV(TXEN0) | _BV(RXEN0);
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
}

void ADC_init(){
	ADMUX=(1<<REFS0);// For Aref=AVcc;
	ADCSRA=(1<<ADEN)|(7<<ADPS0);
}

void eeprom_init(){ // fabrieksinstellingen...
	if(eeprom_read_byte((uint8_t *)100) != 1){
		eeprom_update_byte(0, 150);// maximale afstandmeter lengte = 150cm
		eeprom_update_byte(1, 5);// minimale afstandmeter lengte = 5cm
		eeprom_update_byte(2, 24);//toggle voor automodes: temperatuur = 24 C
		eeprom_update_byte(3, 90);// toggle voor automodes: licht = 90
		eeprom_update_byte(4, 0);// toggle type voor automodus = handmatig
		eeprom_update_byte(100, 1);// als een arduino nooit een eeprom init heeft gehad, bij standaard istellingen kun je 100 op 0 zetten en dan de eeprom_init opnieuw aanvragen.
		
	}	
}