  
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  String formattedDate = timeClient.getFormattedDate();
  config.updated = formattedDate;
  
  // We need to extract date and time
  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  dayStamp = dayStamp.substring(5);
  
  
