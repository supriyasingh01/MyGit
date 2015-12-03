void read_config(char *myconfig)
{
	FILE *confg;
	cfg=fopen(my.conf,"rt");
	char buf[50];
	//char buf1[50];
	bzero(buf,sizeof(buf));
	//bzero(buf1,sizeof(buf1));
	
	while(!feof(confg))
	{
		fscanf(confg,"%s",buf);
		if(feof(confg))
		break;	
		//++entrycnt;

		inet_aton(buf,&Real[entrycnt].dest);
	
		fscanf(cfg,"%s",buf1);
		inet_aton(buf1,&Real[entrycnt].next);
		
		fscanf(cfg,"%s",buf);
		inet_aton(buf,&Real[entrycnt].mask);
		fscanf(cfg,"%s",Real[entrycnt].interface);
		Real[entrycnt].interfaceposn=interfaceentry(Real[entrycnt].interface);
		if(Real[entrycnt].next!=0)
		arpentry(buf1,Real[entrycnt].interface,Real[entrycnt].next,1,entrycnt);
		
	}
	bzero(buf,sizeof(buf));
	fclose(confg);

}