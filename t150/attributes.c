static inline int t150_init_attributes(struct t150 *t150, struct usb_interface *uif)
{
	device_create_file(&uif->dev, &dev_attr_return_force);
	device_create_file(&uif->dev, &dev_attr_use_return_force);
	device_create_file(&uif->dev, &dev_attr_range);
	device_create_file(&uif->dev, &dev_attr_ffb_intensity);

	return 0;
}

static inline void t150_free_attributes(struct t150 *t150, struct usb_interface *uif)
{
	device_remove_file(&uif->dev, &dev_attr_return_force);
	device_remove_file(&uif->dev, &dev_attr_use_return_force);
	device_remove_file(&uif->dev, &dev_attr_range);
	device_remove_file(&uif->dev, &dev_attr_ffb_intensity);
}

/**/
static ssize_t t150_store_return_force(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	uint8_t nforce;
	struct usb_interface *uif = to_usb_interface(dev);
	struct t150 *t150 = usb_get_intfdata(uif);

	char *set = kzalloc(4, GFP_KERNEL);

	// If mallformed input leave...
	if(kstrtou8(buf, 10, &nforce))
		goto ret;

	if(nforce > 100)
		nforce = 100;

	t150_settings_set40(t150, SET40_RETURN_FORCE, nforce, set);

ret:	kfree(set);
	return count;
}

static ssize_t t150_show_return_force(struct device *dev, struct device_attribute *attr,char * buf )
{
	// TODO Read from wheel with USB request
	return sscanf(buf, no_str);
}

static ssize_t t150_store_simulate_return_force(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	bool use;
	struct usb_interface *uif = to_usb_interface(dev);
	struct t150 *t150 = usb_get_intfdata(uif);

	char *set = kzalloc(4, GFP_KERNEL);

	// If mallformed input leave...
	if(!kstrtobool(buf, &use))
		t150_settings_set40(t150, SET40_USE_RETURN_FORCE, use, set);

	kfree(set);
	return count;
}

static ssize_t t150_store_range(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	uint16_t range;
	struct usb_interface *uif = to_usb_interface(dev);
	struct t150 *t150 = usb_get_intfdata(uif);

	char *set = kzalloc(4, GFP_KERNEL);

	// If mallformed input leave...
	if(kstrtou16(buf, 10, &range))
		goto ret;

	if(range < 270)
		range = 270;
	else if (range > 1080)
		range = 1080;

	range = (range * 0xffff) / 1080;

	t150_settings_set40(t150, SET40_RANGE, range, set);

ret:	kfree(set);
	return count;
}

static ssize_t t150_show_range(struct device *dev, struct device_attribute *attr,char * buf )
{
	// TODO Read from wheel with USB request
	return sscanf(buf, no_str);
}

static ssize_t t150_store_ffb_intensity(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	uint8_t nforce;
	struct usb_interface *uif = to_usb_interface(dev);
	struct t150 *t150 = usb_get_intfdata(uif);

	char *set = kzalloc(4, GFP_KERNEL);

	// If mallformed input leave...
	if(kstrtou8(buf, 10, &nforce))
		goto ret;

	if(nforce > 100)
		nforce = 100;

	nforce = (nforce * 0x80)/100;

	t150_settings_set43(t150, nforce, set);

ret:	kfree(set);
	return count;
}

static ssize_t t150_show_ffb_intensity(struct device *dev, struct device_attribute *attr,char * buf )
{
	// TODO Read from wheel with USB request
	return sscanf(buf, no_str);
}
