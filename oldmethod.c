       if(strstr(buf, "adduser") || strstr(buf, "ADDUSER"))
    {
      if(strcmp(admin, accounts[find_line].id) == 0)
      {
        char *token = strtok(buf, " ");
        char *userinfo = token+sizeof(token);
        trim(userinfo);
        char *uinfo[50];
        sprintf(uinfo, "echo '%s' >> kosha.txt", userinfo);
        system(uinfo);
        printf("\x1b[1;37m[\x1b[0;31mKosha\x1b[1;37m] \x1b[1;37mUser:[\x1b[0;36m%s\x1b[1;37m] Added User:[\x1b[0;36m%s\x1b[1;37m]\n", accounts[find_line].user, userinfo);
        sprintf(panel, "\x1b[1;37m[\x1b[0;31mKosha\x1b[1;37m] User:[\x1b[0;36m%s\x1b[1;37m] Successfully Added!\r\n", userinfo);
        if(send(thefd, panel, strlen(panel), MSG_NOSIGNAL) == -1) return;
      }
      else
      {
        sprintf(panel, "\x1b[0;31mAdmins Only!\r\n");
        if(send(thefd, panel, strlen(panel), MSG_NOSIGNAL) == -1);
      }
RAW Paste Data
          if(strstr(buf, "adduser") || strstr(buf, "ADDUSER"))
    {
      if(strcmp(admin, accounts[find_line].id) == 0)
      {
        char *token = strtok(buf, " ");
        char *userinfo = token+sizeof(token);
        trim(userinfo);
        char *uinfo[50];
        sprintf(uinfo, "echo '%s' >> kosha.txt", userinfo);
        system(uinfo);
        printf("\x1b[1;37m[\x1b[0;31mKosha\x1b[1;37m] \x1b[1;37mUser:[\x1b[0;36m%s\x1b[1;37m] Added User:[\x1b[0;36m%s\x1b[1;37m]\n", accounts[find_line].user, userinfo);
        sprintf(panel, "\x1b[1;37m[\x1b[0;31mKosha\x1b[1;37m] User:[\x1b[0;36m%s\x1b[1;37m] Successfully Added!\r\n", userinfo);
        if(send(thefd, panel, strlen(panel), MSG_NOSIGNAL) == -1) return;
      }
      else
      {
        sprintf(panel, "\x1b[0;31mAdmins Only!\r\n");
        if(send(thefd, panel, strlen(panel), MSG_NOSIGNAL) == -1);
      }
