static inline int t150_init_attributes(struct t150 *t150)
{
	int errno;

	// Before making avaible the syfs attrbiutes we try to set them to some default
	// @FIXME I do not know if it is desiradable to wait for URBs in probe() method...
	t150_setup_task(t150);

	errno = device_create_file(&t150->hid_device->dev, &dev_attr_autocenter);
	if(errno)
		return errno;

	errno = device_create_file(&t150->hid_device->dev, &dev_attr_enable_autocenter);
	if(errno)
		goto err1;

	errno = device_create_file(&t150->hid_device->dev, &dev_attr_range);
	if(errno)
		goto err2;

	errno = device_create_file(&t150->hid_device->dev, &dev_attr_gain);
	if(errno)
		goto err3;

	errno = device_create_file(&t150->hid_device->dev, &dev_attr_firmware_version);
	if(errno)
		goto err4;

	return 0;

err4:	device_remove_file(&t150->hid_device->dev, &dev_attr_firmware_version);
err3:	device_remove_file(&t150->hid_device->dev, &dev_attr_range);
err2:	device_remove_file(&t150->hid_device->dev, &dev_attr_enable_autocenter);
err1:	device_remove_file(&t150->hid_device->dev, &dev_attr_autocenter);
	return errno;
}

static inline void t150_free_attributes(struct t150 *t150)
{
	device_remove_file(&t150->hid_device->dev, &dev_attr_autocenter);
	device_remove_file(&t150->hid_device->dev, &dev_attr_enable_autocenter);
	device_remove_file(&t150->hid_device->dev, &dev_attr_range);
	device_remove_file(&t150->hid_device->dev, &dev_attr_gain);
	device_remove_file(&t150->hid_device->dev, &dev_attr_firmware_version);
}

/**/
static ssize_t t150_store_return_force(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	uint8_t nforce;
	struct t150 *t150 = dev_get_drvdata(dev);

	// If mallformed input leave...
	if(kstrtou8(buf, 10, &nforce))
		return count;

	if(nforce > 100)
		nforce = 100;

	t150_set_autocenter(t150, nforce);

	return count;
}

static ssize_t t150_show_return_force(struct device *dev, struct device_attribute *attr,char * buf )
{
	int len;
	struct t150 *t150 = dev_get_drvdata(dev);
	unsigned long flags;

	spin_lock_irqsave(&t150->settings.access_lock, flags);
	len = sprintf(buf, "%d\n", t150->settings.autocenter_force);
	spin_unlock_irqrestore(&t150->settings.access_lock, flags);

	return len;
}

static ssize_t t150_store_simulate_return_force(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	bool use;
	struct t150 *t150 = dev_get_drvdata(dev);

	// If mallformed input leave...
	if(!kstrtobool(buf, &use))
		t150_set_enable_autocenter(t150, use);	

	return count;
}

static ssize_t t150_show_simulate_return_force(struct device *dev, struct device_attribute *attr,char * buf)
{
	int len;
	struct t150 *t150 = dev_get_drvdata(dev);
	unsigned long flags;
	spin_lock_irqsave(&t150->settings.access_lock, flags);
	len = sprintf(buf, "%c\n", t150->settings.autocenter_enabled ? 'y' : 'n');
	spin_unlock_irqrestore(&t150->settings.access_lock, flags);

	return len;
}

static ssize_t t150_store_range(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	uint16_t range;
	int dev_max_range;

	struct t150 *t150 = dev_get_drvdata(dev);

	// If mallformed input leave...
	if(kstrtou16(buf, 10, &range))
		return count;

	if(t150->hid_device->product == USB_T150_PRODUCT_ID)
		dev_max_range = 1080;
	else if (t150->hid_device->product == USB_TMX_PRODUCT_ID)
		dev_max_range = 900;

	if(range < 270)
		range = 270;
	else if (range > dev_max_range)
		range = dev_max_range;

	range = DIV_ROUND_CLOSEST((range * 0xffff), dev_max_range);

	t150_set_range(t150, range);

	return count;
}

static ssize_t t150_show_range(struct device *dev, struct device_attribute *attr,char * buf )
{
	int len;
	struct t150 *t150 = dev_get_drvdata(dev);
	unsigned long flags;

	int dev_max_range;

	if(t150->hid_device->product == USB_T150_PRODUCT_ID)
		dev_max_range = 1080;
	else if (t150->hid_device->product == USB_TMX_PRODUCT_ID)
		dev_max_range = 900;

	spin_lock_irqsave(&t150->settings.access_lock, flags);
	len = sprintf(buf, "%d\n", DIV_ROUND_CLOSEST(t150->settings.range * dev_max_range, 0xffff));
	spin_unlock_irqrestore(&t150->settings.access_lock, flags);

	return len;
}

static ssize_t t150_store_ffb_intensity(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	uint8_t nforce;
	struct t150 *t150 = dev_get_drvdata(dev);

	// If mallformed input leave...
	if(kstrtou8(buf, 10, &nforce))
		return count;

	if(nforce > 100)
		nforce = 100;

	nforce = DIV_ROUND_CLOSEST(nforce * 0x80, 100);

	t150_set_gain(t150, nforce);
	return count;
}

static ssize_t t150_show_ffb_intensity(struct device *dev, struct device_attribute *attr,char * buf )
{
	int len;
	struct t150 *t150 = dev_get_drvdata(dev);
	unsigned long flags;

	spin_lock_irqsave(&t150->settings.access_lock, flags);
	len = sprintf(buf, "%d\n", DIV_ROUND_CLOSEST(t150->settings.gain * 100, 0x80));
	spin_unlock_irqrestore(&t150->settings.access_lock, flags);

	return len;
}

ssize_t t150_show_fw_version(struct device *dev, struct device_attribute *attr,char * buf )
{
	int len;
	struct t150 *t150 = dev_get_drvdata(dev);
	unsigned long flags;

	spin_lock_irqsave(&t150->settings.access_lock, flags);
	len = sprintf(buf, "%d\n", t150->settings.firmware_version);
	spin_unlock_irqrestore(&t150->settings.access_lock, flags);

	return len;
}
